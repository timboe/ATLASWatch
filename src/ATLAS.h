// Move on Basalt
#ifdef PBL_RECT
//
#define XCENTRE 72
#define YCENTRE 84
#define X (XCENTRE-90)
#define Y (YCENTRE-90)
#define CLOCKNUDGE 0
//
#else
//
#define XCENTRE 90
#define YCENTRE 90
#define X 0
#define Y 0
#define CLOCKNUDGE 18
//
#endif
#define WIDTH (XCENTRE*2)
#define HEIGHT (YCENTRE*2)

#define ID_END 30
#define EM_END 48
#define HCAL_END 66

#define N_EMHAD 2
#define N_DECAY 4

#define TEXTSIZE 6

#define TEMP_UNIT_C 0
#define TEMP_UNIT_F 1
#define TEMP_UNIT_K 2

void updateWeather();
void tickHandler(struct tm* tickTime, TimeUnits unitsChanged);

Layer* getLayer();
