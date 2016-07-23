
#include "SME_ui_events.h"

using namespace SME::Event::UI;

WindowEvent::WindowEvent(std::string type) : SME::Event::Event(type) {}
MouseEvent::MouseEvent(std::string type) : SME::Event::UI::WindowEvent(type) {}
KeyEvent::KeyEvent(std::string type) : SME::Event::UI::WindowEvent(type) {}


#define c(variant, base) base##variant##Event::base##variant##Event() : base##Event(UIEventKeys::base##variant##Event) {}

c(Close, Window);
c(Resize, Window);
c(Maximise, Window);
c(Minimise, Window);
c(Up, Mouse);
c(Down, Mouse);
c(Move, Mouse);
c(Wheel, Mouse);
c(Down, Key);
c(Up, Key);
