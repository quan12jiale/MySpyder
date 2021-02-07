#include "terminal.h"

ANSIEscapeCodeHandler::ANSIEscapeCodeHandler()
{
    this->ANSI_COLORS = {QStringList({"#000000", "#808080"}),
                         QStringList({"#800000", "#ff0000"}),
                         QStringList({"#008000", "#00ff00"}),
                         QStringList({"#808000", "#ffff00"}),
                         QStringList({"#000080", "#0000ff"}),
                         QStringList({"#800080", "#ff00ff"}),
                         QStringList({"#008080", "#00ffff"}),
                         QStringList({"#c0c0c0", "#ffffff"})};

    this->intensity = 0;
    this->foreground_color = -1;
    this->background_color = -1;
    this->default_foreground_color = 30;
    this->default_background_color = 47;
}

void ANSIEscapeCodeHandler::set_code(int code)
{
    if (code == 0)
        this->reset();
    else if (code == 1)
        this->intensity = 1;
    else if (code == 3)
        this->italic = true;
    else if (code == 4)
        this->underline = true;
    else if (code == 22) {
        this->intensity = 0;
        this->bold = false;
    }
    else if (code == 23)
        this->italic = false;
    else if (code == 24)
        this->underline = false;
    else if (code >= 30 && code <= 37)
        this->foreground_color = code;
    else if (code == 39)
        this->foreground_color = this->default_foreground_color;
    else if (code >= 40 && code <= 47)
        this->background_color = code;
    else if (code == 49)
        this->background_color = this->default_background_color;
    this->set_style();
}

void ANSIEscapeCodeHandler::reset()
{
    this->current_format = QTextCharFormat();
    this->intensity = 0;
    this->italic = false;
    this->bold = false;
    this->underline = false;
    this->foreground_color = -1;
    this->background_color = -1;
}
