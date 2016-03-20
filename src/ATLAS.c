#include <pebble.h>
#include <ATLAS.h>
#include <communication.h>
#include <gpath_builder.h>

static Window* s_mainWindow;
static Layer* s_atlasLayer;

static const uint32_t const segments[] = { 200, 200, 200, 200, 200 };
VibePattern btDisconnect = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};

const GPoint centre = {
  .x = XCENTRE,
  .y = YCENTRE
};

const GRect timeRect = {
  .origin = { .x = (6+CLOCKNUDGE), .y = YCENTRE-35 },
  .size = { .w = WIDTH-(6+CLOCKNUDGE)*2, .h = 100 }
};

const GPathInfo innerMuon = {
  .num_points = 8,
  .points = (GPoint []) {{62+X, 24+Y}, {119+X, 24+Y}, {156+X, 62+Y}, {156+X, 119+Y}, {119+X, 156+Y}, {62+X, 156+Y}, {24+X, 119+Y}, {24+X, 62+Y}}
};

const GPathInfo outerMuon = {
  .num_points = 8,
  .points = (GPoint []) {{56+X, 8+Y}, {125+X, 8+Y}, {172+X, 56+Y}, {172+X, 124+Y}, {125+X, 172+Y}, {56+X, 172+Y}, {8+X, 124+Y}, {8+X, 56+Y}}
};

// EM large MB deposit, will be moved so that 0,0 is the centre for rotation
const GPathInfo emLarge = {
  .num_points = 6,
  .points = (GPoint []) {{37,2}, {38,3}, {44,3}, {44,-3}, {38,-3}, {37, -2}}
};

const GPathInfo emSmall = {
  .num_points = 6,
  .points = (GPoint []) {{37,1}, {38,2}, {44,2}, {44,-2}, {38,-2}, {37, -1}}
};

const GPathInfo hadLarge = {
  .num_points = 6,
  .points = (GPoint []) {{54,2}, {55,3}, {62,3}, {62,-3}, {55,-3}, {54, -2}}
};

const GPathInfo hadSmall = {
  .num_points = 6,
  .points = (GPoint []) {{55,1}, {56,2}, {61,3}, {61,-3}, {56,-2}, {55,-1}}
};

const GPathInfo electron = {
  .num_points = 4,
  .points = (GPoint []) {{3, ID_END}, {5, ID_END+30}, {-5, ID_END+30}, {-3, ID_END}}
};

const GPathInfo hadJetA = {
  .num_points = 8,
  .points = (GPoint []) {{6, EM_END}, {9, EM_END+40}, {4, EM_END+40}, {3, EM_END+30}, {-1, EM_END+30}, {-3, EM_END+35}, {-9, EM_END+35}, {-6, EM_END}}
};

const GPathInfo hadJetB = {
  .num_points = 8,
  .points = (GPoint []) {{6, EM_END}, {9, EM_END+30}, {4, EM_END+30}, {3, EM_END+35}, {-2, EM_END+35}, {-3, EM_END+40}, {-9, EM_END+42}, {-6, EM_END}}
};

const GPathInfo emJetA = {
  .num_points = 6,
  .points = (GPoint []) {{4, ID_END}, {5, EM_END-3}, {0, EM_END-2}, {0, EM_END+5}, {-5, EM_END+4}, {-4, ID_END}}
};

const GPathInfo emJetB = {
  .num_points = 6,
  .points = (GPoint []) {{4, ID_END}, {5, EM_END+6}, {0, EM_END+5}, {0, EM_END-2}, {-5, EM_END-2}, {-4, ID_END}}
};

static GPath* s_innerMuon;
static GPath* s_outerMuon;
static GPath* s_em[N_EMHAD];
static GPath* s_had[N_EMHAD];
static GPath* s_electronPath;
static GPath* s_hadJetPath[N_EMHAD];
static GPath* s_emJetPath[N_EMHAD];

static int s_muon[N_DECAY] = {-1,-1,-1,-1};
static int s_electron[N_DECAY] = {-1,-1,-1,-1};
static int s_photon[N_DECAY] = {-1,-1,-1,-1};
static int s_bquark[N_DECAY] = {-1,-1,-1,-1};

static char s_clockA[TEXTSIZE];
static char s_clockB[TEXTSIZE];
static char s_dayOfWeek[TEXTSIZE];
static char s_dayOfMonth[TEXTSIZE];

static int s_rndSeed;

static int s_display = false; // are we all setup?

static GFont s_mono;
//static GFont s_comic;

typedef enum {kSun, kMoon, kScatteredDay, kScatteredNight, kCloud,
              kRain, kThunder, kSnow, kFog, kWind} weatherType_t;

static char s_temperature[TEXTSIZE];
static int s_tempValue;
static weatherType_t s_weatherCode;
static bool s_weatherValid = false;

static BatteryChargeState s_battery;
static bool s_bluetoothStatus;
#ifdef PBL_HEALTH
static int s_stepProgress = 0;
#endif

typedef enum {kHBB, kHZZ, kHWW, kHGG, kHiggsDecaySize} higgsDecay_t;
typedef enum {kVEX, kVMuX, kVQQ, kVDecaySize} VDecay_t;

#define N_BITMAP 10
typedef enum {kH, kTo, kZ, kW, kG, kB, kQ, kE, kM, kN} bitmaps_t;
static GBitmap* s_bitmap[N_BITMAP]; // Store of pointers
static GBitmap* s_bitmapBluetooth;
static GBitmap* s_bitmapWeather[N_BITMAP];
#define MAX_STR 10
static GBitmap* s_decayMsg[MAX_STR]; // Writes message

unsigned getN(int rolls, int sides) {
  unsigned _t = 0;
  for (int r = 0; r < rolls; ++r) _t += 1 + (rand() % sides);
  return _t;
}

unsigned fuzzAngle(int angle, int fuzz) {
  fuzz = (TRIG_MAX_ANGLE / 360) * fuzz;
  angle = (rand() % 2 == 0 ? angle + fuzz : angle - fuzz);
  if (angle > TRIG_MAX_ANGLE) angle -= TRIG_MAX_ANGLE;
  else if (angle < 0) angle += TRIG_MAX_ANGLE;
  return (unsigned) angle;
}

unsigned randomAngle(int allign) { // 1=EM, 2=HAD
  int angle = rand() % (TRIG_MAX_ANGLE/60);
  if (allign == 1 && angle % 2 == 0) ++angle;
  else if (allign == 2 && angle % 2 == 1) ++angle;
  return angle * (TRIG_MAX_ANGLE/60);
}

void chooseNewDecay() {

  unsigned _letter = 0;
  s_decayMsg[_letter++] = s_bitmap[kH];
  s_decayMsg[_letter++] = s_bitmap[kTo];

  higgsDecay_t _decay = rand() % kHiggsDecaySize;

  int _a[2];
  _a[0] = randomAngle(0);
  _a[1] = fuzzAngle(_a[0] + (TRIG_MAX_ANGLE/4), 40);

  if (_decay == kHBB) {
    s_bquark[0] = _a[0];
    s_bquark[1] = fuzzAngle(_a[0] + (TRIG_MAX_ANGLE/2), 20);
    s_decayMsg[_letter++] = s_bitmap[kB];
    s_decayMsg[_letter++] = s_bitmap[kB];
  } else if (_decay == kHGG) {
    s_photon[0] = _a[0];
    s_photon[1] = fuzzAngle(_a[0] + (TRIG_MAX_ANGLE/2), 20);
    s_decayMsg[_letter++] = s_bitmap[kG];
    s_decayMsg[_letter++] = s_bitmap[kG];
  } else if (_decay == kHWW) {
    int _nE = 0, _nMu = 0, _nQ = 0;
    s_decayMsg[_letter++] = s_bitmap[kW];
    s_decayMsg[_letter++] = s_bitmap[kW];
    s_decayMsg[_letter++] = s_bitmap[kTo];
    for (int d=0; d < 2; ++d) {
      VDecay_t _w = rand() % kVDecaySize;
      if (_w == kVEX) {
        s_electron[_nE++] = _a[d];
        // Neutrino - invisible
        s_decayMsg[_letter++] = s_bitmap[kE];
        s_decayMsg[_letter++] = s_bitmap[kN];
      } else if (_w == kVMuX) {
        s_muon[_nMu++] = _a[d];
        // Neutrino - invisible
        s_decayMsg[_letter++] = s_bitmap[kM];
        s_decayMsg[_letter++] = s_bitmap[kN];
      } else if (_w == kVQQ) {
        s_bquark[_nQ++] = _a[d];
        s_bquark[_nQ++] = fuzzAngle(_a[d] + (TRIG_MAX_ANGLE/2), 20);
        s_decayMsg[_letter++] = s_bitmap[kQ];
        s_decayMsg[_letter++] = s_bitmap[kQ];
      }
    }
  } else if (_decay == kHZZ) {
    int _nE = 0, _nMu = 0, _nQ = 0;
    s_decayMsg[_letter++] = s_bitmap[kZ];
    s_decayMsg[_letter++] = s_bitmap[kZ];
    s_decayMsg[_letter++] = s_bitmap[kTo];
    for (int d=0; d < 2; ++d) {
      VDecay_t _z = rand() % kVDecaySize;
      if (_z == kVEX) {
        s_electron[_nE++] = _a[d];
        s_electron[_nE++] = fuzzAngle(_a[d] + (TRIG_MAX_ANGLE/2), 20);
        s_decayMsg[_letter++] = s_bitmap[kE];
        s_decayMsg[_letter++] = s_bitmap[kE];
      } else if (_z == kVMuX) {
        s_muon[_nMu++] = _a[d];
        s_muon[_nMu++] = fuzzAngle(_a[d] + (TRIG_MAX_ANGLE/2), 20);
        s_decayMsg[_letter++] = s_bitmap[kM];
        s_decayMsg[_letter++] = s_bitmap[kM];
      } else if (_z == kVQQ) {
        s_bquark[_nQ++] = _a[d];
        s_bquark[_nQ++] = fuzzAngle(_a[d] + (TRIG_MAX_ANGLE/2), 20);
        s_decayMsg[_letter++] = s_bitmap[kQ];
        s_decayMsg[_letter++] = s_bitmap[kQ];
      }
    }
  }
}

void updateWeather() {
  if (persist_read_int(OPT_WEATHER) == 0) return;

  // Check buffer
  int32_t _timeOfWeather = persist_read_int(DATA_WEATHER_TIME);
  int32_t _now = time(NULL);

  if (_timeOfWeather == 0 || _timeOfWeather > _now || (_now - _timeOfWeather) > 3*SECONDS_PER_HOUR) {

    requestWeatherUpdate(); // Needs refreshing

  } else { // use cache

    int32_t _tempType = persist_read_int(OPT_TEMP_UNIT);
    s_tempValue = persist_read_int(DATA_WEATHER_TEMP);
    s_weatherCode = persist_read_int(DATA_WEATHER_ICON);

    if (_tempType == TEMP_UNIT_F) {
      s_tempValue = ((s_tempValue*5)/9) + 32;
      snprintf(s_temperature, sizeof(s_temperature), "%i", s_tempValue);
      strcat(s_temperature, "F");
    } else if (_tempType == TEMP_UNIT_C) {
      snprintf(s_temperature, sizeof(s_temperature), "%i", s_tempValue);
      strcat(s_temperature, "C");
    } else {
      s_tempValue += 273;
      snprintf(s_temperature, sizeof(s_temperature), "%i", s_tempValue);
      strcat(s_temperature, "K");
    }

    s_weatherValid = true;
    layer_mark_dirty(s_atlasLayer);

  }
}

void updateBattery(BatteryChargeState charge) {
  if (persist_read_int(OPT_BATTERY) == 0) return;
  s_battery = charge;
  layer_mark_dirty(s_atlasLayer);
}

void updateBluetooth(bool bluetooth) {
  if (persist_read_int(OPT_BLUETOOTH) == 0) {
    s_bluetoothStatus = true;
    return;
  }
  s_bluetoothStatus = bluetooth;
  if (s_bluetoothStatus == false) vibes_enqueue_custom_pattern(btDisconnect);
  layer_mark_dirty(s_atlasLayer);
}

#ifdef PBL_HEALTH
void updateSteps() {
  if (persist_read_int(OPT_ACTIVITY) == 0) return;
  HealthMetric _metric = HealthMetricStepCount;
  time_t _start = time_start_of_today();
  time_t _endNow = time(NULL);
  time_t _endDay = _start + (24 * SECONDS_PER_HOUR);
  // Check the metric has data available for today
  HealthServiceAccessibilityMask _maskS = health_service_metric_accessible(_metric, _start, _endNow);
  HealthServiceAccessibilityMask _maskA = health_service_metric_averaged_accessible(_metric, _start, _endDay, HealthServiceTimeScopeDailyWeekdayOrWeekend);

  if((_maskA & HealthServiceAccessibilityMaskAvailable) && (_maskS & HealthServiceAccessibilityMaskAvailable)) {
    int _stepsToday = (int) health_service_sum_today(_metric);
    int _stepsAverage = (int) health_service_sum_averaged(_metric, _start, _endDay, HealthServiceTimeScopeDailyWeekdayOrWeekend);

    s_stepProgress = (TRIG_MAX_ANGLE * _stepsToday) / _stepsAverage;
  }
}
#endif

void draw3DText(GContext *ctx, GRect loc, GFont f, const char* buffer, uint8_t offset, GTextAlignment al, GColor fg, GColor bg) {
  graphics_context_set_text_color(ctx, bg);

  loc.origin.x -= offset; // CL
  loc.origin.y += offset; // UL
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);

  loc.origin.x += offset; // CU
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);
  loc.origin.x += offset; // RU
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);

  loc.origin.y -= offset; // CR
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);
  loc.origin.y -= offset; // DR
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);

  loc.origin.x -= offset; // DC
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);
  loc.origin.x -= offset; // DR
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);

  loc.origin.y += offset; // CR
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);

  // main
  graphics_context_set_text_color(ctx, fg);
  loc.origin.x += offset; // O
  graphics_draw_text(ctx, buffer, f, loc, GTextOverflowModeWordWrap, al, NULL);
}

GFont getFont() {
  return s_mono;
}

void tickHandler(struct tm* tickTime, TimeUnits unitsChanged) {

  for (unsigned i=0; i < N_DECAY; ++i) {
    s_muon[i] = -1;
    s_electron[i] = -1;
    s_photon[i] = -1;
    s_bquark[i] = -1;
  }
  for (unsigned i=0; i < MAX_STR; ++i) {
    s_decayMsg[i] = NULL;
  }

  APP_LOG(APP_LOG_LEVEL_INFO,"UnitsChanged is %i, hour unit is %i, cond is %i", (int)unitsChanged, (int)HOUR_UNIT, (int)(unitsChanged & HOUR_UNIT) );

  if ((unitsChanged & HOUR_UNIT) > 0) { // Check date
    APP_LOG(APP_LOG_LEVEL_INFO,"tick chk weather");
    updateWeather();
    strftime(s_dayOfWeek, sizeof(s_dayOfWeek), "%a", tickTime);
    strftime(s_dayOfMonth, sizeof(s_dayOfMonth), "%d", tickTime);
    for (int i=0; i < TEXTSIZE; ++i) {
      if (s_dayOfWeek[i] >= 'a' && s_dayOfWeek[i] <= 'z') s_dayOfWeek[i] -= 'a' - 'A'; //ASCII manipulation FTW (toupper)
    }
  }

  if (persist_read_int(OPT_ANALOGUE) == 1) { // analogue

    unsigned _letter = 0;
    s_decayMsg[_letter++] = s_bitmap[kH];
    s_decayMsg[_letter++] = s_bitmap[kTo];
    s_decayMsg[_letter++] = s_bitmap[kW];
    s_decayMsg[_letter++] = s_bitmap[kW];
    s_decayMsg[_letter++] = s_bitmap[kTo];
    s_decayMsg[_letter++] = s_bitmap[kE];
    s_decayMsg[_letter++] = s_bitmap[kN];
    s_decayMsg[_letter++] = s_bitmap[kM];
    s_decayMsg[_letter++] = s_bitmap[kN];

    s_muon[0] = (TRIG_MAX_ANGLE * tickTime->tm_min) / 60; // Min
    int _h = tickTime->tm_hour;
    if (_h >= 12) _h -= 12;
    s_electron[0] = ((TRIG_MAX_ANGLE * _h) / 12) + (s_muon[0] / 12); // Hour
    // TODO - the electron is off by pi, fix
    s_electron[0] += TRIG_MAX_ANGLE/2;

    // Only update background junk on the hour
    if ((unitsChanged & HOUR_UNIT) > 0) s_rndSeed = time(NULL);

  } else { // digital

    if(clock_is_24h_style() == true) {
      strftime(s_clockA, sizeof(s_clockA), "%H", tickTime);
    } else {
      strftime(s_clockA, sizeof(s_clockA), "%I", tickTime);
    }
    strftime(s_clockB, sizeof(s_clockB), "%M", tickTime);
    chooseNewDecay();
    s_rndSeed = time(NULL);

  }

  // update the step count on the hour. If 11PM then update the average step count
  #ifdef PBL_HEALTH
  if (unitsChanged & HOUR_UNIT) updateSteps();
  #endif

  layer_mark_dirty(s_atlasLayer);
}

static void drawCurvyLine(GContext* ctx, int rInner, int rOuter, int pT, int angle) {
  const int sina = sin_lookup(angle);
  const int cosa = -cos_lookup(angle);

  const int rInSina = (rInner * sina)/TRIG_MAX_RATIO;
  const int rInCosa = (rInner * cosa)/TRIG_MAX_RATIO;
  const int rOutSina = (rOuter * sina)/TRIG_MAX_RATIO;
  const int rOutCosa = (rOuter * cosa)/TRIG_MAX_RATIO;

  const int sign = rand() % 2 == 0 ? 1 : -1;
  const int pTSina = sign * (pT * sina)/TRIG_MAX_RATIO;
  const int pTCosa = sign * (pT * cosa)/TRIG_MAX_RATIO;

  GPoint _start = GPoint( XCENTRE + rInSina,  YCENTRE + rInCosa);
  GPoint _end   = GPoint( XCENTRE + rOutSina, YCENTRE + rOutCosa);

  GPoint _bezOffset1 = GPoint( XCENTRE + rInSina  + pTCosa, YCENTRE + rInCosa  - pTSina);
  GPoint _bezOffset2 = GPoint( XCENTRE + rOutSina + pTCosa, YCENTRE + rOutCosa - pTSina);

  GPathBuilder* _builder = gpath_builder_create(256);
  gpath_builder_move_to_point(_builder, _start);
  gpath_builder_curve_to_point(_builder, _end, _bezOffset1, _bezOffset2);
  GPath* _path = gpath_builder_create_path(_builder);
  gpath_draw_outline_open(ctx, _path);
  gpath_builder_destroy(_builder);
  gpath_destroy(_path);

}

static void drawLine(GContext* ctx, int rInner, int rOuter, int angle) {
  const int sina = sin_lookup(angle);
  const int cosa = -cos_lookup(angle);

  const int rInSina = (rInner * sina)/TRIG_MAX_RATIO;
  const int rInCosa = (rInner * cosa)/TRIG_MAX_RATIO;
  const int rOutSina = (rOuter * sina)/TRIG_MAX_RATIO;
  const int rOutCosa = (rOuter * cosa)/TRIG_MAX_RATIO;

  GPoint _start = GPoint( XCENTRE + rInSina,  YCENTRE + rInCosa);
  GPoint _end   = GPoint( XCENTRE + rOutSina, YCENTRE + rOutCosa);

  graphics_draw_line(ctx, _start, _end);
}

static void atlasUpdateProc(Layer* thisLayer, GContext *ctx) {

  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  srand(s_rndSeed);

  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorBlue,GColorBlack));
  gpath_draw_filled(ctx, s_outerMuon);
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, s_innerMuon);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorRed,GColorBlack));
  graphics_fill_circle(ctx, centre, HCAL_END);
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorGreen,GColorBlack));
  graphics_fill_circle(ctx, centre, EM_END);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, centre, ID_END);

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 3);
#ifdef PBL_HEALTH
  if (persist_read_int(OPT_ACTIVITY == 1)) {
    graphics_context_set_stroke_width(ctx, 1);
  }
#endif
  graphics_draw_circle(ctx, centre, ID_END);
  graphics_draw_circle(ctx, centre, EM_END);
  graphics_draw_circle(ctx, centre, HCAL_END);
#ifdef PBL_HEALTH
  // Do health
  if (persist_read_int(OPT_ACTIVITY == 1)) {
    graphics_context_set_stroke_width(ctx, 3);
    GRect _rID = GRect(centre.x - ID_END, centre.y - ID_END, 2 * ID_END, 2 * ID_END);
    GRect _rEM = GRect(centre.x - EM_END, centre.y - EM_END, 2 * EM_END, 2 * EM_END);
    GRect _rHC = GRect(centre.x - HCAL_END, centre.y - HCAL_END, 2 * HCAL_END, 2 * HCAL_END);
    graphics_draw_arc(ctx, _rID, GOvalScaleModeFitCircle, 0, s_stepProgress);
    graphics_draw_arc(ctx, _rEM, GOvalScaleModeFitCircle, 0, s_stepProgress);
    graphics_draw_arc(ctx, _rHC, GOvalScaleModeFitCircle, 0, s_stepProgress);
  }
#endif
  graphics_context_set_stroke_width(ctx, 3);
  gpath_draw_outline(ctx, s_innerMuon);
  gpath_draw_outline(ctx, s_outerMuon);

  // Muon cross lines
  graphics_context_set_stroke_width(ctx, 1);
  for (int i=0; i < (int) innerMuon.num_points; ++i) {
    graphics_draw_line(ctx, innerMuon.points[i], outerMuon.points[i]);
  }

  // Calo cross lines
  int _c = 0;
  for (unsigned a=0; a < TRIG_MAX_ANGLE; a += TRIG_MAX_ANGLE/60) {
    if (++_c % 2 == 0) {
      drawLine(ctx, ID_END, EM_END, a);
    } else {
      drawLine(ctx, EM_END, HCAL_END, a);
    }
  }

  if (s_display == true) {

    // Minbias
    #define PT_MAX 15
    graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorCyan,GColorWhite));
    for (unsigned i=0; i < getN(6, 8); ++i) { // Tracks
      drawCurvyLine(ctx, 0, 30, rand() % PT_MAX, rand() % TRIG_MAX_ANGLE);
    }
    graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorPastelYellow,GColorWhite));
    for (unsigned i=0; i < getN(3, 4); ++i) { // EM calo
      int angle = randomAngle(1);
      int size = rand() % N_EMHAD;
      gpath_rotate_to(s_em[size], angle);
      gpath_draw_filled(ctx, s_em[size]);
    }
    for (unsigned i=0; i < getN(2, 4); ++i) { // HAD calo
      int angle = randomAngle(2);
      int size = rand() % N_EMHAD;
      gpath_rotate_to(s_had[size], angle);
      gpath_draw_filled(ctx, s_had[size]);
    }

    // Electron
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorYellow,GColorWhite));
    graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorDarkGreen,GColorLightGray));
    for (int i=0; i < N_DECAY; ++i) {
      if (s_electron[i] == -1) break;
      gpath_rotate_to(s_electronPath, s_electron[i]);
      gpath_draw_filled(ctx, s_electronPath);
      drawLine(ctx, 0, ID_END, s_electron[i] + TRIG_MAX_ANGLE/2);
    }

    // Photon
    for (int i=0; i < N_DECAY; ++i) {
      if (s_photon[i] == -1) break;
      gpath_rotate_to(s_electronPath, s_photon[i]);
      gpath_draw_filled(ctx, s_electronPath);
    }

    // Jet
    for (int i=0; i < N_DECAY; ++i) {
      if (s_bquark[i] == -1) break;
      int _r1 = rand() % N_EMHAD;
      int _r2 = rand() % N_EMHAD;
      gpath_rotate_to(s_hadJetPath[_r1], s_bquark[i]);
      gpath_rotate_to(s_emJetPath[_r2], s_bquark[i]);
      graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorDarkCandyAppleRed,GColorLightGray));
      gpath_draw_filled(ctx, s_hadJetPath[_r1]);
      graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorDarkGreen,GColorLightGray));
      gpath_draw_filled(ctx, s_emJetPath[_r2]);
      for (unsigned j=0; j < getN(4,2); ++j) {
        drawLine(ctx, 0, ID_END, fuzzAngle(s_bquark[i] + TRIG_MAX_ANGLE/2, 10));
      }
    }

    // Muon
    graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorRed, GColorWhite));
    graphics_context_set_stroke_width(ctx, 3);
    for (int i=0; i < N_DECAY; ++i) {
      if (s_muon[i] == -1) break;
      drawLine(ctx, 0, 200, s_muon[i]);
    }

    // DECAY MSG
    if (persist_read_int(OPT_DECAY) == 1) {
      unsigned _msgSize = 0;
      for (unsigned i=0; i < MAX_STR; ++i) {
        if (s_decayMsg[i] == NULL) break;
        ++_msgSize;
      }
      GRect _bitmapBox = GRect(XCENTRE - (_msgSize*20)/7 /*tweak 5/2*/, PBL_IF_ROUND_ELSE(12,6), 5, 7);
      for (unsigned i=0; i < MAX_STR; ++i) {
        if (s_decayMsg[i] == NULL) break;
        graphics_draw_bitmap_in_rect(ctx, s_decayMsg[i], _bitmapBox);
        _bitmapBox.origin.x += 6;
      }
    }

  } // if s_display == true

  // TIME
  if (persist_read_int(OPT_ANALOGUE) == 0) {
    draw3DText(ctx, timeRect, getFont(), s_clockA, 2, GTextAlignmentLeft, GColorWhite, GColorBlack);
    draw3DText(ctx, timeRect, getFont(), s_clockB, 2, GTextAlignmentRight, GColorWhite, GColorBlack);
  }

  // DATE
  int l = PBL_IF_ROUND_ELSE(27,2), w = 30, h = 34;
  graphics_context_set_text_color(ctx, GColorBlack);
  if (persist_read_int(OPT_CALENDAR) == 1) {
    graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorDarkGray,GColorBlack));
    graphics_fill_rect(ctx, GRect(l,l,w,h), 4, GCornersAll);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(l+2,l+2,w-4,h-4), 4, GCornersAll);
    graphics_draw_text(ctx, s_dayOfWeek, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(l, l+1, w, h), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, s_dayOfMonth, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(l, l+11, w, h), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }

  // WEATHER
  if (s_weatherValid == true && persist_read_int(OPT_WEATHER) == 1) {
    graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorDarkGray,GColorBlack));
    graphics_fill_rect(ctx, GRect(WIDTH-w-l,l,w,h), 4, GCornersAll);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(WIDTH-w-l+2,l+2,w-4,h-4), 4, GCornersAll);
    graphics_draw_text(ctx, s_temperature, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(WIDTH-w-l, l-1, w, h), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    graphics_draw_bitmap_in_rect(ctx, s_bitmapWeather[ s_weatherCode ], GRect(WIDTH-w-l+3, l+14, 24, 17));
  }

  // BATTERY
  if (persist_read_int(OPT_BATTERY) == 1) {
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 1);
    GPoint _b = GPoint(XCENTRE-9, HEIGHT - PBL_IF_ROUND_ELSE(20, 14) );
    graphics_draw_rect(ctx, GRect(_b.x       , _b.y      , 18, 9));
    graphics_draw_rect(ctx, GRect(_b.x + 18  , _b.y + 2  , 2, 5));
    graphics_fill_rect(ctx, GRect(_b.x + 2  ,  _b.y + 2  ,s_battery.charge_percent/7, 5), 0, GCornersAll); // 100%=14 pixels
  }

  // BLUETOOTH
  if (persist_read_int(OPT_BLUETOOTH) == 1 && s_bluetoothStatus == false) {
    w = 24;
    h = 35;
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(XCENTRE-12,YCENTRE+30,w,h), 4, GCornersAll);
    graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorDarkCandyAppleRed,GColorBlack));
    graphics_fill_rect(ctx, GRect(XCENTRE-12+2,YCENTRE+30+2,w-4,h-4), 4, GCornersAll);
    graphics_draw_bitmap_in_rect(ctx, s_bitmapBluetooth, GRect(XCENTRE-11+3,YCENTRE+30+3, 16,29));
  }
}

static void mainWindowLoad(Window* window) {
  // Get information about the Window
  Layer* windowLayer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(windowLayer);

  // Create the TextLayer with specific bounds
  s_atlasLayer = layer_create(bounds);
  layer_add_child(windowLayer, s_atlasLayer);
  layer_set_update_proc(s_atlasLayer, atlasUpdateProc);

  s_innerMuon = gpath_create(&innerMuon);
  s_outerMuon = gpath_create(&outerMuon);
  s_em[0] = gpath_create(&emSmall);
  s_em[1] = gpath_create(&emLarge);
  s_had[0] = gpath_create(&hadSmall);
  s_had[1] = gpath_create(&hadLarge);
  s_electronPath = gpath_create(&electron);
  s_hadJetPath[0] = gpath_create(&hadJetA);
  s_hadJetPath[1] = gpath_create(&hadJetB);
  s_emJetPath[0] = gpath_create(&emJetA);
  s_emJetPath[1] = gpath_create(&emJetB);
  for (int s=0; s < N_EMHAD; ++s) {
    gpath_move_to(s_em[s], centre);
    gpath_move_to(s_had[s], centre);
    gpath_move_to(s_hadJetPath[s], centre);
    gpath_move_to(s_emJetPath[s], centre);
  }
  gpath_move_to(s_electronPath, centre);

  s_mono = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MONO_49));
  //s_comic = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_COMIC_49));

  s_bitmap[kH]  = gbitmap_create_with_resource(RESOURCE_ID_H);
  s_bitmap[kTo] = gbitmap_create_with_resource(RESOURCE_ID_TO);
  s_bitmap[kW]  = gbitmap_create_with_resource(RESOURCE_ID_W);
  s_bitmap[kZ]  = gbitmap_create_with_resource(RESOURCE_ID_Z);
  s_bitmap[kG]  = gbitmap_create_with_resource(RESOURCE_ID_G);
  s_bitmap[kB]  = gbitmap_create_with_resource(RESOURCE_ID_B);
  s_bitmap[kQ]  = gbitmap_create_with_resource(RESOURCE_ID_Q);
  s_bitmap[kE]  = gbitmap_create_with_resource(RESOURCE_ID_E);
  s_bitmap[kM]  = gbitmap_create_with_resource(RESOURCE_ID_M);
  s_bitmap[kN]  = gbitmap_create_with_resource(RESOURCE_ID_N);

  s_bitmapWeather[kSun]  = gbitmap_create_with_resource(RESOURCE_ID_SUN);
  s_bitmapWeather[kMoon] = gbitmap_create_with_resource(RESOURCE_ID_MOON);
  s_bitmapWeather[kScatteredDay]  = gbitmap_create_with_resource(RESOURCE_ID_SCATTEREDDAY);
  s_bitmapWeather[kScatteredNight]  = gbitmap_create_with_resource(RESOURCE_ID_SCATTEREDNIGHT);
  s_bitmapWeather[kCloud]  = gbitmap_create_with_resource(RESOURCE_ID_CLOUD);
  s_bitmapWeather[kRain]  = gbitmap_create_with_resource(RESOURCE_ID_RAIN);
  s_bitmapWeather[kSnow]  = gbitmap_create_with_resource(RESOURCE_ID_SNOW);
  s_bitmapWeather[kThunder]  = gbitmap_create_with_resource(RESOURCE_ID_THUNDER);
  s_bitmapWeather[kWind]  = gbitmap_create_with_resource(RESOURCE_ID_WIND);
  s_bitmapWeather[kFog]  = gbitmap_create_with_resource(RESOURCE_ID_FOG);

  s_bitmapBluetooth = gbitmap_create_with_resource(RESOURCE_ID_BT);

  battery_state_service_subscribe(updateBattery);
  s_battery = battery_state_service_peek();
  bluetooth_connection_service_subscribe(updateBluetooth);
  s_bluetoothStatus = bluetooth_connection_service_peek();

  // Make sure the time and steps are displayed from the start
  PBL_IF_HEALTH_ELSE( updateSteps(false), /*noop*/ {} );
  time_t _t = time(NULL);
  struct tm* _time = localtime(&_t);
  tickHandler(_time, HOUR_UNIT);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tickHandler);
  s_display = true;
}

static void mainWindowUnload(Window *window) {
  // Destroy TextLayer
  layer_destroy(s_atlasLayer);

  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  destroyCommunication();

  gpath_destroy(s_innerMuon);
  gpath_destroy(s_outerMuon);
  gpath_destroy(s_electronPath);
  for (int s=0; s < N_EMHAD; ++s) {
    gpath_destroy(s_em[s]);
    gpath_destroy(s_had[s]);
    gpath_destroy(s_hadJetPath[s]);
    gpath_destroy(s_emJetPath[s]);
  }
  fonts_unload_custom_font(s_mono);
  //fonts_unload_custom_font(s_comic);
  for (int b=0; b < N_BITMAP; ++b) {
    gbitmap_destroy(s_bitmap[b]);
    gbitmap_destroy(s_bitmapWeather[b]);
  }
  gbitmap_destroy(s_bitmapBluetooth);
}


static void init() {
  s_mainWindow = window_create();

  if (persist_exists(OPT_ANALOGUE) == false) persist_write_int(OPT_ANALOGUE, 0);
  if (persist_exists(OPT_TEMP_UNIT) == false) persist_write_int(OPT_TEMP_UNIT, TEMP_UNIT_C);
  if (persist_exists(OPT_CALENDAR) == false) persist_write_int(OPT_CALENDAR, 1);
  if (persist_exists(OPT_WEATHER) == false) persist_write_int(OPT_WEATHER, 1);
  if (persist_exists(OPT_BATTERY) == false) persist_write_int(OPT_BATTERY, 1);
  if (persist_exists(OPT_DECAY) == false) persist_write_int(OPT_DECAY, 1);
  if (persist_exists(OPT_ACTIVITY) == false) persist_write_int(OPT_ACTIVITY, 1);
  if (persist_exists(OPT_BLUETOOTH) == false) persist_write_int(OPT_BLUETOOTH, 1);
  if (persist_exists(DATA_WEATHER_TEMP) == false) persist_write_int(DATA_WEATHER_TEMP, 0);
  if (persist_exists(DATA_WEATHER_ICON) == false) persist_write_int(DATA_WEATHER_ICON, 0);
  if (persist_exists(DATA_WEATHER_TIME) == false) persist_write_int(DATA_WEATHER_TIME, 0);

  registerCommunication();

  window_set_window_handlers(s_mainWindow, (WindowHandlers) {
    .load = mainWindowLoad,
    .unload = mainWindowUnload
  });
  window_set_background_color(s_mainWindow, GColorBlack);
  window_stack_push(s_mainWindow, true);

}

static void deinit() {
  // Destroy Window
  window_destroy(s_mainWindow);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
