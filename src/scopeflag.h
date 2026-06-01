#ifndef SCOPEFLAG_H
#define SCOPEFLAG_H

/* Tiny RAII guard for a bool flag: sets it true for the lifetime of the guard
 * and restores its PREVIOUS value on scope exit. Use it to bracket "we're in a
 * programmatic operation, suppress interactive side-effects" sections (config
 * loads, etc.) so an early return can't leave the flag stuck set.
 *
 *   {
 *       BoolFlagGuard g(m_configLoadInProgress);
 *       ... // flag is true here
 *   }   // flag restored to its prior value here
 *
 * Restores the prior value (not hard-false) so nested guards behave. */
class BoolFlagGuard
{
public:
    explicit BoolFlagGuard(bool &flag) : m_flag(flag), m_prev(flag) { m_flag = true; }
    ~BoolFlagGuard() { m_flag = m_prev; }

    BoolFlagGuard(const BoolFlagGuard &) = delete;
    BoolFlagGuard &operator=(const BoolFlagGuard &) = delete;

private:
    bool &m_flag;
    bool m_prev;
};

#endif // SCOPEFLAG_H
