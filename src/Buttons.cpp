#include <functional>
#include <FunctionalInterrupt.h>
#include "Buttons.h"

#ifdef ONE_BUTTON
Button::Button(uint8_t pin, bool level, const EventQueue *events) : _isrtime(0), _events((EventQueue*)events) {
  _item.pin = (pin == 16) ? GPIO16_RENUM : pin;
  _item.level = level;
  _item.paused = false;
  _item.pressed = false;
  _item.dblclickable = false;
  _item.duration = 0;
  pinMode(pin, level ? INPUT : INPUT_PULLUP);
  attachInterrupt(pin, [this]() { this->_isr(this); }, CHANGE);
}
#endif

#ifndef ONE_BUTTON
uint8_t Buttons::add(uint8_t pin, bool level) {
  if (pin > 16)
    return ERR_INDEX;

  uint8_t result;
  _button_t b;

  b.pin = (pin == 16) ? GPIO16_RENUM : pin;
  b.level = level;
  b.paused = false;
  b.pressed = false;
  b.dblclickable = false;
  b.duration = 0;
  result = List<_button_t, 10>::add(b);
  if (result != ERR_INDEX) {
    pinMode(pin, level ? INPUT : INPUT_PULLUP);
    attachInterrupt(pin, [this]() { this->_isr(this); }, CHANGE);
  }

  return result;
}
#endif

#ifdef ONE_BUTTON
void Button::pause() {
  _item.paused = true;
  detachInterrupt(pinToGpio(_item.pin));
}
#else
void Buttons::pause(uint8_t index) {
  if (_items && (index < _count)) {
    _items[index].paused = true;
    detachInterrupt(pinToGpio(_items[index].pin));
  }
}
#endif

#ifdef ONE_BUTTON
void Button::resume() {
  _item.paused = false;
  _item.pressed = false;
  _item.dblclickable = false;
  _item.duration = 0;
  attachInterrupt(pinToGpio(_item.pin), [this]() { this->_isr(this); }, CHANGE);
}
#else
void Buttons::resume(uint8_t index) {
  if (_items && (index < _count)) {
    _items[index].paused = false;
    _items[index].pressed = false;
    _items[index].dblclickable = false;
    _items[index].duration = 0;
    attachInterrupt(pinToGpio(_items[index].pin), [this]() { this->_isr(this); }, CHANGE);
  }
}
#endif

#ifdef ONE_BUTTON
void ICACHE_RAM_ATTR Button::_isr(Button *_this) {
  if (! _this->_item.paused) {
    uint32_t time = millis() - _this->_isrtime;
    uint32_t inputs = GPI;

    if (_this->_item.duration) {
      if (time + _this->_item.duration < 0xFFFF)
        _this->_item.duration += time;
      else
        _this->_item.duration = 0xFFFF;
    }
    if (((inputs >> pinToGpio(_this->_item.pin)) & 0x01) == _this->_item.level) { // Button pressed
      if (! _this->_item.pressed) {
        _this->_item.dblclickable = (_this->_item.duration > 0) && (_this->_item.duration <= DBLCLICK_TIME);
        _this->_item.pressed = true;
        _this->_item.duration = 1;
        _this->onChange(BTN_PRESSED);
      }
    } else { // Button released
      if (_this->_item.pressed) { // Was pressed
        if (_this->_item.duration >= LONGCLICK_TIME) {
          _this->onChange(BTN_LONGCLICK);
        } else if (_this->_item.duration >= CLICK_TIME) {
          if (_this->_item.dblclickable)
            _this->onChange(BTN_DBLCLICK);
          else
            _this->onChange(BTN_CLICK);
        } else {
          _this->onChange(BTN_RELEASED);
        }
        _this->_item.pressed = false;
        if ((_this->_item.duration >= CLICK_TIME) && (_this->_item.duration < LONGCLICK_TIME))
          _this->_item.duration = 1;
        else
          _this->_item.duration = 0;
      }
    }
  }
  _this->_isrtime = millis();
}
#else
void ICACHE_RAM_ATTR Buttons::_isr(Buttons *_this) {
  if (_this->_items && _this->_count) {
    uint32_t time = millis() - _this->_isrtime;
    uint32_t inputs = GPI;

    for (uint8_t i = 0; i < _this->_count; ++i) {
      if (_this->_items[i].paused)
        continue;
      if (_this->_items[i].duration) {
        if (time + _this->_items[i].duration < 0xFFFF)
          _this->_items[i].duration += time;
        else
          _this->_items[i].duration = 0xFFFF;
      }
      if (((inputs >> pinToGpio(_this->_items[i].pin)) & 0x01) == _this->_items[i].level) { // Button pressed
        if (! _this->_items[i].pressed) {
          _this->_items[i].dblclickable = (_this->_items[i].duration > 0) && (_this->_items[i].duration <= DBLCLICK_TIME);
          _this->_items[i].pressed = true;
          _this->_items[i].duration = 1;
          _this->onChange(BTN_PRESSED, i);
        }
      } else { // Button released
        if (_this->_items[i].pressed) { // Was pressed
          if (_this->_items[i].duration >= LONGCLICK_TIME) {
            _this->onChange(BTN_LONGCLICK, i);
          } else if (_this->_items[i].duration >= CLICK_TIME) {
            if (_this->_items[i].dblclickable)
              _this->onChange(BTN_DBLCLICK, i);
            else
              _this->onChange(BTN_CLICK, i);
          } else {
            _this->onChange(BTN_RELEASED, i);
          }
          _this->_items[i].pressed = false;
          if ((_this->_items[i].duration >= CLICK_TIME) && (_this->_items[i].duration < LONGCLICK_TIME))
            _this->_items[i].duration = 1;
          else
            _this->_items[i].duration = 0;
        }
      }
    }
  }
  _this->_isrtime = millis();
}
#endif

#ifndef ONE_BUTTON
void Buttons::cleanup(void *ptr) {
  detachInterrupt(pinToGpio(((_button_t*)ptr)->pin));
}

bool Buttons::match(uint8_t index, const void *t) {
  if (_items && (index < _count)) {
    return (_items[index].pin == ((_button_t*)t)->pin);
  }

  return false;
}
#endif

#ifdef ONE_BUTTON
void Button::onChange(buttonstate_t state) {
#else
void Buttons::onChange(buttonstate_t state, uint8_t button) {
#endif
  if (_events) {
    event_t e;

    e.id = EVT_BTNBASE + state;
#ifdef ONE_BUTTON
    e.data = 0;
#else
    e.data = button;
#endif
    _events->put(&e, true);
  }
}
