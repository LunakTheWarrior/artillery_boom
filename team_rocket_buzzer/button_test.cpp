#include "button.h"
#include <cstdlib>

void check(bool condition) { if (!condition) std::abort(); }

int main() {
    Button b;
    check(!b.update(true, 0));
    check(!b.update(true, 100)); // Held at boot must not play.
    check(!b.update(false, 110));
    check(!b.update(false, 140));
    check(!b.update(true, 150));
    check(!b.update(false, 155)); // Contact bounce.
    check(!b.update(true, 160));
    check(!b.update(true, 189));
    check(b.update(true, 190));
    check(!b.update(true, 1000)); // No repeat while held.
    check(!b.update(false, 1010));
    check(!b.update(true, 1020)); // Short release must not re-arm.
    check(!b.update(true, 1060));
    check(!b.update(false, 1070));
    check(!b.update(false, 1100));
    check(!b.update(true, 1110));
    check(b.update(true, 1140));
    b = Button{}; // Same reset as playback completion.
    check(!b.update(true, 2000));
    check(!b.update(true, 2100));
    check(!b.update(false, 2200));
    check(!b.update(false, 2230));
    check(!b.update(true, 2240));
    check(b.update(true, 2270));
}
