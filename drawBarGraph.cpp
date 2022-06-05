// 
// 
// 

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "drawBarGraph.h"

// Thanks to https://github.com/KrisKasprzak

/*-----------------------------------------------------------------*/

/*
  This method will draw a vertical bar graph for single input
  it has a rather large arguement list and is as follows
  &d = display object name
  x = position of bar graph (lower left of bar)
  y = position of bar (lower left of bar
  w = width of bar graph
  h =  height of bar graph (does not need to be the same as the max scale)
  loval = lower value of the scale (can be negative)
  hival = upper value of the scale
  inc = scale division between loval and hival
  curval = date to graph (must be between loval and hival)
  dig = format control to set number of digits to display (not includeing the decimal)
  dec = format control to set number of decimals to display (not includeing the decimal)
  barcolor = color of bar graph
  voidcolor = color of bar graph background
  bordercolor = color of the border of the graph
  textcolor = color of the text
  backcolor = color of the bar graph's background
  label = bottom lable text for the graph
  redraw = flag to redraw display only on first pass (to reduce flickering)
*/

void drawBarChartV1(Adafruit_ILI9341& tft, byte screen, double x, double y, double w, double h, double loval, double hival, double inc, double curval, int dig, int dec, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, String label, boolean fillBar, boolean& redraw)
{

    double stepval, range;
    double my, level;
    double i, data;
    // draw the border, scale, and label once
    // avoid doing this on every update to minimize flicker
    if (redraw == true) {
        redraw = true;

        tft.drawRect(x - 1, y - h - 1, w + 2, h + 2, bordercolor);
        tft.setTextColor(textcolor, backcolor);
        tft.setTextSize(1);
        tft.setCursor(x + 0, y + 7);
        tft.println(label);
        // step val basically scales the hival and low val to the height
        // deducting a small value to eliminate round off errors
        // this val may need to be adjusted
        stepval = (inc) * (double(h) / (double(hival - loval))) - .001;
        for (i = 0; i <= h; i += stepval) {
            my = y - h + i;
            //tft.drawFastHLine(x + w + 1, my,  5, textcolor);
            // draw lables
            //tft.setTextSize(1);
            //tft.setTextColor(BLACK, backcolor);
            //tft.setCursor(x + w + 12, my - 3 );
            //data = hival - ( i * (inc / stepval));
            //tft.println(Format(data, dig, dec));
            //tft.setFont();
        }
    }
    // compute level of bar graph that is scaled to the  height and the hi and low vals
    // this is needed to accompdate for +/- range
    level = (h * (((curval - loval) / (hival - loval))));
    // draw the bar graph
    // write a upper and lower bar to minimize flicker cause by blanking out bar and redraw on update
    tft.fillRect(x, y - h, w, h - level, voidcolor);
    tft.drawFastHLine(x, y + 1 - level, w, barcolor);
    tft.drawFastHLine(x, y + 1 - level - 1, w, barcolor);
    //tft.drawRect(x - 1, y - h - 1, w + 2, h + 2, bordercolor);
    
    if (fillBar == 1) {

        tft.fillRect(x, y + 1 - level, w,  level, barcolor);
    }
    
    else {}

    tft.drawRect(x - 1, y - h - 1, w + 2, h + 2, bordercolor);

    // write the current value
    /*
      tft.setTextColor(textcolor, backcolor);
      tft.setTextSize(1);
      tft.setCursor(x , y - h - 23);
      tft.println(Format(curval, dig, dec));
    */
}

/*-----------------------------------------------------------------*/

void drawBarChartV2(Adafruit_ILI9341& tft, byte screen, double x, double y, double w, double h, double loval, double hival, double inc, double curval, int dig, int dec, unsigned int barcolor, unsigned int voidcolor, unsigned int bordercolor, unsigned int textcolor, unsigned int backcolor, String label, boolean fillBar, boolean& redraw)
{

    double stepval, range;
    double my, level;
    double i, data;
    // draw the border, scale, and label once
    // avoid doing this on every update to minimize flicker
    if (redraw == true) {
        redraw = true;

        tft.drawRect(x - 1, y - h - 1, w + 2, h + 2, bordercolor);
        tft.setTextColor(textcolor, backcolor);
        tft.setTextSize(1);
        tft.setCursor(x + 0, y + 7);
        tft.println(label);
        // step val basically scales the hival and low val to the height
        // deducting a small value to eliminate round off errors
        // this val may need to be adjusted
        stepval = (inc) * (double(h) / (double(hival - loval))) - .001;
        for (i = 0; i <= h; i += stepval) {
            my = y - h + i;
            tft.drawFastHLine(x + w + 1, my, 5, bordercolor);
            // draw lables
            tft.setTextSize(1);
            tft.setTextColor(textcolor, backcolor);
            tft.setCursor(x + w + 12, my - 3);
            data = hival - (i * (inc / stepval));
            tft.println(Format(data, dig, dec));
            tft.setFont();
        }
    }
    // compute level of bar graph that is scaled to the  height and the hi and low vals
    // this is needed to accompdate for +/- range
    level = (h * (((curval - loval) / (hival - loval))));
    // draw the bar graph
    // write a upper and lower bar to minimize flicker cause by blanking out bar and redraw on update
    tft.fillRect(x, y - h, w, h - level, voidcolor);
    tft.drawFastHLine(x, y + 1 - level, w, barcolor);
    tft.drawFastHLine(x, y + 1 - level - 1, w, barcolor);
    //tft.drawRect(x - 1, y - h - 1, w + 2, h + 2, bordercolor);

    if (fillBar == 1) {

        tft.fillRect(x, y + 1 - level, w, level, barcolor);
    }

    else {}

    tft.drawRect(x - 1, y - h - 1, w + 2, h + 2, bordercolor);

    // write the current value
    /*
      tft.setTextColor(textcolor, backcolor);
      tft.setTextSize(1);
      tft.setCursor(x , y - h - 23);
      tft.println(Format(curval, dig, dec));
    */
}

/*-----------------------------------------------------------------*/

String Format(double val, int dec, int dig) {
    int addpad = 0;
    char sbuf[20];
    String condata = (dtostrf(val, dec, dig, sbuf));


    int slen = condata.length();
    for (addpad = 1; addpad <= dec + dig - slen; addpad++) {
        condata = " " + condata;
    }
    return (condata);

}

/*-----------------------------------------------------------------*/