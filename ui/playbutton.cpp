#include "playbutton.h"

PlayButton::PlayButton(QWidget* parent) : QPushButton(parent)
{
	play_text = ">";
	pause_text = "||";

	setText(play_text);
}
