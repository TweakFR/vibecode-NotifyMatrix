#include "idfm_client.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <cstdlib>
#include <ctime>
#include <cstring>

static String idfm_uri_encode_component(const char* s)
{
  String o;
  if (s == nullptr) {
    return o;
  }
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(s); *p != '\0'; p++) {
    const unsigned char c = *p;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
        c == '_' || c == '.' || c == '~') {
      o += static_cast<char>(c);
    } else {
      char buf[5];
      snprintf(buf, sizeof(buf), "%%%02X", (unsigned)c);
      o += buf;
    }
  }
  return o;
}

namespace {

void copy_cstr(char* dst, size_t dst_size, const char* src)
{
  if (dst_size == 0) {
    return;
  }
  if (src == nullptr) {
    src = "";
  }
  std::strncpy(dst, src, dst_size - 1);
  dst[dst_size - 1] = '\0';
}

bool is_configured(const char* value)
{
  return value != nullptr && value[0] != '\0' && std::strcmp(value, "VOTRE_CLE_PRIM") != 0;
}

int64_t days_from_civil(int y, unsigned m, unsigned d)
{
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

bool parse_int(const char* s, size_t pos, size_t len, int& out)
{
  int value = 0;
  for (size_t i = 0; i < len; i++) {
    const char c = s[pos + i];
    if (c < '0' || c > '9') {
      return false;
    }
    value = value * 10 + (c - '0');
  }
  out = value;
  return true;
}

bool parse_iso8601_epoch(const char* iso, time_t& out)
{
  if (iso == nullptr || std::strlen(iso) < 19) {
    return false;
  }

  int y = 0;
  int mo = 0;
  int d = 0;
  int h = 0;
  int mi = 0;
  int sec = 0;
  if (!parse_int(iso, 0, 4, y) || !parse_int(iso, 5, 2, mo) ||
      !parse_int(iso, 8, 2, d) || !parse_int(iso, 11, 2, h) ||
      !parse_int(iso, 14, 2, mi) || !parse_int(iso, 17, 2, sec)) {
    return false;
  }

  int offset_seconds = 0;
  const char* tz = iso + 19;
  while (*tz != '\0' && *tz != 'Z' && *tz != '+' && *tz != '-') {
    tz++;
  }
  if (*tz == '+' || *tz == '-') {
    int oh = 0;
    int om = 0;
    if (std::strlen(tz) < 6 || !parse_int(tz, 1, 2, oh) || !parse_int(tz, 4, 2, om)) {
      return false;
    }
    offset_seconds = (oh * 3600 + om * 60) * (*tz == '+' ? 1 : -1);
  }

  const int64_t days = days_from_civil(y, (unsigned)mo, (unsigned)d);
  out = (time_t)(days * 86400 + h * 3600 + mi * 60 + sec - offset_seconds);
  return true;
}

const char* first_string(JsonVariant value)
{
  if (value.is<const char*>()) {
    return value.as<const char*>();
  }
  if (value.is<JsonArray>()) {
    JsonVariant first = value[0];
    if (first["value"].is<const char*>()) {
      return first["value"].as<const char*>();
    }
  }
  if (value["value"].is<const char*>()) {
    return value["value"].as<const char*>();
  }
  return nullptr;
}

const char* siri_datetime_string(JsonVariant v)
{
  if (v.is<const char*>()) {
    return v.as<const char*>();
  }
  if (v.is<JsonObject>()) {
    JsonObject o = v.as<JsonObject>();
    if (o["value"].is<const char*>()) {
      return o["value"].as<const char*>();
    }
  }
  return nullptr;
}

const char* first_time_value(JsonObject call)
{
  static const char* const kKeys[] = {"ExpectedDepartureTime",
                                       "ExpectedArrivalTime",
                                       "AimedDepartureTime",
                                       "AimedArrivalTime",
                                       "ActualDepartureTime",
                                       "ActualArrivalTime"};
  for (const char* k : kKeys) {
    const char* s = siri_datetime_string(call[k]);
    if (s != nullptr && s[0] != '\0') {
      return s;
    }
  }
  return nullptr;
}

/// LineRef PRIM : soit chaîne directe, soit objet { "value": "STIF:..." }.
const char* journey_line_ref_cstr(JsonObject journey)
{
  JsonVariant lr = journey["LineRef"];
  if (lr.isNull()) {
    return "";
  }
  if (lr.is<const char*>()) {
    const char* s = lr.as<const char*>();
    return s != nullptr ? s : "";
  }
  if (lr.is<JsonObject>()) {
    JsonObject o = lr.as<JsonObject>();
    if (o["value"].is<const char*>()) {
      const char* s = o["value"].as<const char*>();
      return s != nullptr ? s : "";
    }
  }
  return "";
}

bool label_is_all_ascii_digits(const char* s)
{
  if (s == nullptr || s[0] == '\0') {
    return false;
  }
  for (const char* p = s; *p != '\0'; p++) {
    if (*p < '0' || *p > '9') {
      return false;
    }
  }
  return true;
}

/// Libellé .env (ex. "0628", "2225") vs LineRef STIF (ex. STIF:Line::C00628:).
/// Les codes réseau sont souvent "C" + 5 chiffres ; "0628" -> cherche "C00628".
bool line_label_matches_stif_line_ref(const char* lr, const char* label)
{
  if (lr == nullptr || lr[0] == '\0' || label == nullptr || label[0] == '\0') {
    return false;
  }
  if (std::strstr(lr, label) != nullptr) {
    return true;
  }
  if (label_is_all_ascii_digits(label)) {
    const int n = std::atoi(label);
    if (n > 0 && n <= 99999) {
      char code[12];
      snprintf(code, sizeof(code), "C%05d", n);
      if (std::strstr(lr, code) != nullptr) {
        return true;
      }
    }
  }
  return false;
}

bool journey_matches_line_filter(JsonObject journey,
                                 const char* line_ref,
                                 const char* line_code_substr,
                                 const char* line_label_for_pub)
{
  const bool has_ref = line_ref != nullptr && line_ref[0] != '\0';
  const bool has_code = line_code_substr != nullptr && line_code_substr[0] != '\0';
  const bool has_label = line_label_for_pub != nullptr && line_label_for_pub[0] != '\0';

  const char* lr = journey_line_ref_cstr(journey);

  if (has_ref && lr[0] != '\0') {
    if (std::strcmp(lr, line_ref) == 0) {
      return true;
    }
    if (std::strlen(line_ref) >= 5 && std::strstr(lr, line_ref) != nullptr) {
      return true;
    }
  }

  // route_id type IDFM:C00628 -> souvent STIF:Line::C00628: dans LineRef SIRI
  if (has_code && has_label) {
    if (lr[0] == '\0' || std::strstr(lr, line_code_substr) == nullptr) {
      return false;
    }
    // LineRef (ex. STIF:Line::C00628:) identifie la ligne ; PublishedLineName est souvent
    // absent ou sans le numéro commercial — ne pas l'exiger quand le code STIF matche.
    return true;
  }

  if (has_code) {
    return lr[0] != '\0' && std::strstr(lr, line_code_substr) != nullptr;
  }

  if (has_label) {
    const char* pub = first_string(journey["PublishedLineName"]);
    if (pub != nullptr && pub[0] != '\0' && std::strstr(pub, line_label_for_pub) != nullptr) {
      return true;
    }
    if (line_label_matches_stif_line_ref(lr, line_label_for_pub)) {
      return true;
    }
    return false;
  }

  return !has_ref;
}

/// Un élément MonitoredStopVisit (SIRI : tableau ou objet unique).
void idfm_consider_one_visit(JsonObject visit,
                             const char* line_ref,
                             const char* line_code_substr,
                             const char* fallback_line_label,
                             time_t now,
                             int& best_minutes,
                             const char*& best_label,
                             unsigned& skipped_line,
                             unsigned& skipped_bad_iso,
                             unsigned& skipped_past,
                             unsigned& kept_line)
{
  JsonObject journey = visit["MonitoredVehicleJourney"];
  if (journey.isNull()) {
    return;
  }
  if (!journey_matches_line_filter(journey, line_ref, line_code_substr, fallback_line_label)) {
    skipped_line++;
    return;
  }
  kept_line++;

  const char* label = first_string(journey["PublishedLineName"]);
  if (label == nullptr || label[0] == '\0') {
    label = fallback_line_label;
  }

  JsonObject call = journey["MonitoredCall"];
  const char* departure = first_time_value(call);
  time_t departure_epoch = 0;
  if (!parse_iso8601_epoch(departure, departure_epoch)) {
    skipped_bad_iso++;
    if (skipped_bad_iso <= 2u) {
      Serial.printf("[IDFM] dbg skip_bad_iso dep=%.40s\n", departure != nullptr ? departure : "(null)");
    }
    return;
  }

  const long delta_seconds = (long)difftime(departure_epoch, now);
  if (delta_seconds < -60) {
    skipped_past++;
    return;
  }
  int minutes = (int)((delta_seconds + 59) / 60);
  if (minutes < 0) {
    minutes = 0;
  }
  if (minutes < best_minutes) {
    best_minutes = minutes;
    best_label = label;
  }
}

void set_error(IdfmResult& result, const char* text, int http_code = 0)
{
  result.ok = false;
  result.minutes = -1;
  result.http_code = http_code;
  copy_cstr(result.text, sizeof(result.text), "ERR");
  copy_cstr(result.error, sizeof(result.error), text);
}

/// Corps HTTP / JSON long : tronque pour le port série (max_print plafonné en interne).
void idfm_serial_trunc(const char* prefix, const char* body, size_t max_print = 220)
{
  constexpr size_t kBufCap = 512;
  if (body == nullptr) {
    Serial.printf("%s (null)\n", prefix);
    return;
  }
  if (max_print > kBufCap - 1) {
    max_print = kBufCap - 1;
  }
  const size_t n = std::strlen(body);
  if (n <= max_print) {
    Serial.printf("%s%s\n", prefix, body);
    return;
  }
  char buf[kBufCap];
  std::memcpy(buf, body, max_print);
  buf[max_print] = '\0';
  Serial.printf("%s%s… (total %u octets)\n", prefix, buf, (unsigned)n);
}

/// Indique qu’une clé est présente sans l’afficher en clair (longueur + 2 premiers / 2 derniers caractères).
void idfm_log_api_key_meta(const char* api_key)
{
  if (api_key == nullptr || api_key[0] == '\0') {
    Serial.println("[IDFM] entete apikey: (vide)");
    return;
  }
  const size_t n = std::strlen(api_key);
  if (n <= 4u) {
    Serial.printf("[IDFM] entete apikey: len=%u (cle tres courte — verifier .env)\n", (unsigned)n);
    return;
  }
  Serial.printf("[IDFM] entete apikey: len=%u debut=%.2s...fin=%.2s\n",
                (unsigned)n,
                api_key,
                api_key + (n - 2));
}

/// Imprime une chaîne longue (ex. URL) par morceaux pour éviter les soucis de buffer Serial.printf.
void idfm_serial_print_cstr_chunks(const char* label, const char* s)
{
  Serial.print(label);
  if (s == nullptr) {
    Serial.println("(null)");
    return;
  }
  for (size_t i = 0; s[i] != '\0'; i += 120) {
    char piece[121];
    size_t j = 0;
    for (; j < 120 && s[i + j] != '\0'; j++) {
      piece[j] = s[i + j];
    }
    piece[j] = '\0';
    Serial.print(piece);
  }
  Serial.println();
}

/// PRIM renvoie parfois `StopMonitoringDelivery` comme tableau `[{ ... }]`,
/// parfois comme un seul objet `{ ... }`. Le filtre Json `[0]` exclut alors tout le contenu.
JsonObject idfm_get_stop_monitoring_delivery(JsonVariant smd)
{
  if (smd.isNull()) {
    return JsonObject();
  }
  if (smd.is<JsonArray>()) {
    JsonArray arr = smd.as<JsonArray>();
    if (arr.size() == 0) {
      return JsonObject();
    }
    return arr[0].as<JsonObject>();
  }
  if (smd.is<JsonObject>()) {
    return smd.as<JsonObject>();
  }
  return JsonObject();
}

} // namespace

bool idfm_fetch_next_departure(const char* api_key,
                               const char* monitoring_ref,
                               const char* line_ref,
                               const char* line_code_substr,
                               const char* fallback_line_label,
                               IdfmResult& result)
{
  const uint32_t t_start_ms = millis();
  result = IdfmResult{};
  copy_cstr(result.line, sizeof(result.line), fallback_line_label);

  Serial.printf("[IDFM] --- requete ms=%lu heap=%u epoch=%lld\n",
                (unsigned long)t_start_ms,
                (unsigned)ESP.getFreeHeap(),
                (long long)time(nullptr));

  if (!is_configured(api_key) || !is_configured(monitoring_ref)) {
    Serial.printf("[IDFM] FAIL missing_config key_ok=%d ref_ok=%d\n",
                  is_configured(api_key) ? 1 : 0,
                  is_configured(monitoring_ref) ? 1 : 0);
    set_error(result, "missing config");
    copy_cstr(result.text, sizeof(result.text), "--");
    return false;
  }

  if (time(nullptr) < 1700000000) {
    Serial.printf("[IDFM] FAIL time_not_synced epoch=%lld\n", (long long)time(nullptr));
    set_error(result, "time not synced");
    copy_cstr(result.text, sizeof(result.text), "--");
    return false;
  }

  Serial.printf("[IDFM] ref=%.48s line_ref=%.32s code=%.24s label=%.16s\n",
                monitoring_ref != nullptr ? monitoring_ref : "(null)",
                line_ref != nullptr && line_ref[0] != '\0' ? line_ref : "(vide)",
                line_code_substr != nullptr && line_code_substr[0] != '\0' ? line_code_substr : "(vide)",
                fallback_line_label != nullptr ? fallback_line_label : "(null)");

  String url = "https://prim.iledefrance-mobilites.fr/marketplace/stop-monitoring?MonitoringRef=";
  url += idfm_uri_encode_component(monitoring_ref);
  if (line_ref != nullptr && line_ref[0] != '\0') {
    url += "&LineRef=";
    url += idfm_uri_encode_component(line_ref);
  }

  Serial.println();
  Serial.println(F("[IDFM] ========== APPEL API PRIM =========="));
  Serial.println(F("[IDFM] service: GET https://prim.iledefrance-mobilites.fr/marketplace/stop-monitoring"));
  Serial.println(F("[IDFM] format: JSON (SIRI Lite), entete Accept: application/json"));
  Serial.println(F("[IDFM] TLS: WiFiClientSecure + setInsecure() (pas de pinning certificat)"));
  Serial.printf("[IDFM] HTTPClient timeout=%u ms | heap avant requete=%u\n",
                15000u,
                (unsigned)ESP.getFreeHeap());
  Serial.printf("[IDFM] WiFi: status=%d (3=WL_CONNECTED) RSSI=%d dBm\n",
                (int)WiFi.status(),
                (int)WiFi.RSSI());
  idfm_log_api_key_meta(api_key);
  idfm_serial_print_cstr_chunks("[IDFM] URL (MonitoringRef + LineRef encodes): ", url.c_str());
  Serial.printf("[IDFM] URL longueur=%u caracteres\n", (unsigned)url.length());

  WiFiClientSecure secure_client;
  secure_client.setInsecure();

  HTTPClient http;
  http.setTimeout(15000);
  if (!http.begin(secure_client, url)) {
    Serial.printf("[IDFM] FAIL http_begin(url) WiFi_status=%d heap=%u\n",
                  (int)WiFi.status(),
                  (unsigned)ESP.getFreeHeap());
    Serial.println(F("[IDFM] ========== FIN APPEL (http_begin KO) =========="));
    set_error(result, "http begin");
    return false;
  }

  http.addHeader("accept", "application/json");
  http.addHeader("apikey", api_key);
  Serial.println(F("[IDFM] envoi GET..."));

  const int http_code = http.GET();
  result.http_code = http_code;
  Serial.printf("[IDFM] reponse ligne: HTTP %d (200=OK) dt=%lu ms heap=%u\n",
                http_code,
                (unsigned long)(millis() - t_start_ms),
                (unsigned)ESP.getFreeHeap());
  if (http_code != HTTP_CODE_OK) {
    const String err_body = http.getString();
    Serial.printf("[IDFM] FAIL http_status=%d heap=%u dt=%lums\n",
                  http_code,
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned long)(millis() - t_start_ms));
    idfm_serial_trunc("[IDFM] corps erreur HTTP (debut): ", err_body.c_str(), 400);
    http.end();
    Serial.println(F("[IDFM] ========== FIN APPEL (echec HTTP) =========="));
    set_error(result, "http error", http_code);
    return false;
  }

  // Lire tout le JSON en RAM : le parse depuis getStream() + TLS est souvent incomplet sur
  // ESP32 (désérialisation en échec → "ERR" à l’écran). Réponses stop-monitoring restent
  // petites si IDFM_LINE_REF est renseigné dans .env.
  const String response_body = http.getString();
  http.end();
  Serial.printf("[IDFM] corps lu: %u octets | heap=%u | dt=%lu ms\n",
                (unsigned)response_body.length(),
                (unsigned)ESP.getFreeHeap(),
                (unsigned long)(millis() - t_start_ms));
  if (response_body.length() == 0) {
    Serial.printf("[IDFM] FAIL empty_response_body heap=%u dt=%lums\n",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned long)(millis() - t_start_ms));
    Serial.println(F("[IDFM] ========== FIN APPEL (corps vide) =========="));
    set_error(result, "empty body", http_code);
    return false;
  }

  idfm_serial_trunc("[IDFM] corps JSON (debut): ", response_body.c_str(), 400);

  JsonDocument filter;
  // Garder tout le bloc delivery : si `StopMonitoringDelivery` est un objet (pas un tableau),
  // un filtre sur `[0]` supprime MonitoredStopVisit et le parseur voit un champ null.
  filter["Siri"]["ServiceDelivery"]["StopMonitoringDelivery"] = true;

  JsonDocument doc;
  // JSON PRIM/SIRI : objets imbriques (LineRef.value, tableaux, MonitoredCall…) dépassent
  // la limite ArduinoJson par défaut (10) → erreur TooDeep sans limite plus haute.
  constexpr uint8_t kJsonNestingLimit = 30;
  DeserializationError error = deserializeJson(
      doc,
      response_body,
      DeserializationOption::Filter(filter),
      DeserializationOption::NestingLimit(kJsonNestingLimit));
  if (error) {
    Serial.printf("[IDFM] FAIL json_deserialize code=%s ec=%u nesting_limit=%u heap=%u dt=%lums\n",
                  error.c_str(),
                  (unsigned)error.code(),
                  (unsigned)kJsonNestingLimit,
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned long)(millis() - t_start_ms));
    idfm_serial_trunc("[IDFM] corps rejoue pour debug parse: ", response_body.c_str(), 400);
    Serial.println(F("[IDFM] ========== FIN APPEL (parse JSON KO) =========="));
    set_error(result, "json error", http_code);
    return false;
  }

  JsonObject delivery = idfm_get_stop_monitoring_delivery(
      doc["Siri"]["ServiceDelivery"]["StopMonitoringDelivery"]);
  if (delivery.isNull()) {
    Serial.printf("[IDFM] FAIL stop_monitoring_delivery absent/vide heap=%u dt=%lums\n",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned long)(millis() - t_start_ms));
    Serial.println(F("[IDFM] ========== FIN APPEL (pas de delivery JSON) =========="));
    set_error(result, "no delivery", http_code);
    copy_cstr(result.text, sizeof(result.text), "--");
    return false;
  }
  if (delivery["ErrorCondition"].is<JsonObject>()) {
    JsonObject errInf = delivery["ErrorCondition"]["ErrorInformation"].as<JsonObject>();
    const char* msg = errInf["ErrorText"].as<const char*>();
    if (msg == nullptr || msg[0] == '\0') {
      msg = errInf["ErrorDescription"].as<const char*>();
    }
    if (msg == nullptr) {
      msg = "PRIM ErrorCondition";
    }
    Serial.printf("[IDFM] FAIL api_error_condition: %s (heap=%u)\n",
                  msg,
                  (unsigned)ESP.getFreeHeap());
    Serial.println(F("[IDFM] ========== FIN APPEL (PRIM ErrorCondition) =========="));
    result.ok = false;
    result.http_code = http_code;
    copy_cstr(result.error, sizeof(result.error), msg);
    copy_cstr(result.text, sizeof(result.text), "--");
    return false;
  }

  JsonVariant msv = delivery["MonitoredStopVisit"];
  if (msv.isNull()) {
    Serial.printf("[IDFM] FAIL monitored_stop_visit absent/null heap=%u dt=%lums\n",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned long)(millis() - t_start_ms));
    Serial.println(F("[IDFM] ========== FIN APPEL (pas de passages dans JSON) =========="));
    set_error(result, "no visits", http_code);
    copy_cstr(result.text, sizeof(result.text), "--");
    return false;
  }

  Serial.println(F("[IDFM] JSON structure OK, analyse MonitoredStopVisit / lignes…"));

  const time_t now = time(nullptr);
  int best_minutes = 9999;
  const char* best_label = fallback_line_label;
  unsigned skipped_line = 0;
  unsigned skipped_bad_iso = 0;
  unsigned skipped_past = 0;
  unsigned kept_line = 0;
  size_t visit_count = 0;

  if (msv.is<JsonArray>()) {
    JsonArray visits = msv.as<JsonArray>();
    visit_count = visits.size();
    if (visit_count == 0) {
      Serial.printf("[IDFM] FAIL visits_empty (tableau 0) heap=%u\n", (unsigned)ESP.getFreeHeap());
      Serial.println(F("[IDFM] ========== FIN APPEL (liste passages vide) =========="));
      set_error(result, "no visits", http_code);
      copy_cstr(result.text, sizeof(result.text), "--");
      return false;
    }
    for (JsonObject visit : visits) {
      idfm_consider_one_visit(visit,
                              line_ref,
                              line_code_substr,
                              fallback_line_label,
                              now,
                              best_minutes,
                              best_label,
                              skipped_line,
                              skipped_bad_iso,
                              skipped_past,
                              kept_line);
    }
  } else if (msv.is<JsonObject>()) {
    visit_count = 1;
    idfm_consider_one_visit(msv.as<JsonObject>(),
                            line_ref,
                            line_code_substr,
                            fallback_line_label,
                            now,
                            best_minutes,
                            best_label,
                            skipped_line,
                            skipped_bad_iso,
                            skipped_past,
                            kept_line);
  } else {
    Serial.printf("[IDFM] FAIL monitored_stop_visit type inattendu heap=%u\n",
                  (unsigned)ESP.getFreeHeap());
    Serial.println(F("[IDFM] ========== FIN APPEL (type MonitoredStopVisit inconnu) =========="));
    set_error(result, "no visits", http_code);
    copy_cstr(result.text, sizeof(result.text), "--");
    return false;
  }

  if (best_minutes == 9999) {
    Serial.printf(
        "[IDFM] FAIL no_match visits=%u line_ok=%u line_skip=%u bad_iso=%u past60s=%u "
        "epoch_now=%lld heap=%u dt=%lums\n",
        (unsigned)visit_count,
        kept_line,
        skipped_line,
        skipped_bad_iso,
        skipped_past,
        (long long)now,
        (unsigned)ESP.getFreeHeap(),
        (unsigned long)(millis() - t_start_ms));
    Serial.println(F("[IDFM] ========== FIN APPEL (aucun passage retenu apres filtres) =========="));
    set_error(result, "no match", http_code);
    copy_cstr(result.text, sizeof(result.text), "--");
    return false;
  }

  result.ok = true;
  result.minutes = best_minutes;
  copy_cstr(result.line, sizeof(result.line), best_label);
  snprintf(result.text, sizeof(result.text), "%dm", best_minutes);
  copy_cstr(result.error, sizeof(result.error), "");
  Serial.printf("[IDFM] OK minutes=%d line=%.20s visits=%u heap=%u dt=%lums\n",
                best_minutes,
                best_label != nullptr ? best_label : "?",
                (unsigned)visit_count,
                (unsigned)ESP.getFreeHeap(),
                (unsigned long)(millis() - t_start_ms));
  Serial.printf("[IDFM] UI: texte affiche bus=\"%s\" ok=%d\n", result.text, (int)result.ok);
  Serial.println(F("[IDFM] ========== FIN APPEL (succes) =========="));
  return true;
}
