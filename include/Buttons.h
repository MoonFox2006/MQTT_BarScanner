#ifndef __BUTTONS_H
#define __BUTTONS_H

#define ONE_BUTTON

#ifndef ONE_BUTTON
#include "List.h"
#endif
#include "Events.h"

struct __packed _button_t {
  uint8_t pin : 4;
  bool level : 1;
  bool paused : 1;
  bool pressed : 1;
  bool dblclickable : 1;
  volatile uint16_t duration;
};

const uint8_t EVT_BTNBASE = 0;

enum buttonstate_t : uint8_t { BTN_RELEASED, BTN_PRESSED, BTN_CLICK, BTN_LONGCLICK, BTN_DBLCLICK };

enum btneventid_t : uint8_t { EVT_BTNRELEASED = EVT_BTNBASE + BTN_RELEASED, EVT_BTNPRESSED = EVT_BTNBASE + BTN_PRESSED,
  EVT_BTNCLICK = EVT_BTNBASE + BTN_CLICK, EVT_BTNLONGCLICK = EVT_BTNBASE + BTN_LONGCLICK, EVT_BTNDBLCLICK = EVT_BTNBASE + BTN_DBLCLICK };

#ifdef ONE_BUTTON
class Button {
public:
  Button(uint8_t pin, bool level, const EventQueue *events = NULL);

  void pause();
  void resume();
#else
class Buttons : public List<_button_t, 10> {
public:
  Buttons(const EventQueue *events = NULL) : List<_button_t, 10>(), _isrtime(0), _events((EventQueue*)events) {}

  uint8_t add(uint8_t pin, bool level);
  void pause(uint8_t index);
  void resume(uint8_t index);
#endif

protected:
  static const uint8_t GPIO16_RENUM = 6; // Renum GPIO16 to unused GPIO6

  static const uint16_t CLICK_TIME = 20; // 20 ms. debounce time
  static const uint16_t LONGCLICK_TIME = 2000; // 2 sec.
  static const uint16_t DBLCLICK_TIME = 500; // 0.5 sec.

  static uint8_t pinToGpio(uint8_t pin) {
    return (pin == GPIO16_RENUM) ? 16 : pin;
  }
#ifndef ONE_BUTTON
  void cleanup(void *ptr);
  bool match(uint8_t index, const void *t);
#endif

#ifdef ONE_BUTTON
  static void _isr(Button *_this);
  virtual void onChange(buttonstate_t state);
#else
  static void _isr(Buttons *_this);
  virtual void onChange(buttonstate_t state, uint8_t button);
#endif

#ifdef ONE_BUTTON
  _button_t _item;
#endif
  uint32_t _isrtime;
  EventQueue *_events;
};

#endif
