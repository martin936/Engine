#ifndef ENGINE_TIMESPAN_INC
#define ENGINE_TIMESPAN_INC

#include <stdint.h>

/// TimeSpan: strongly-typed duration / point-in-time used by the engine's
/// time-keeping API (CWindow::GetTime, CEngine::GetFrameDuration / GetEngineTime,
/// CMainGame's game time).
///
/// Stored as int64 microseconds — enough resolution for any frame-rate work
/// and ~2.9e5 years of range, and immune to the float drift that an "elapsed
/// seconds since startup" double slowly accumulates.
///
/// Construction is via named factories so units are always explicit at the
/// call site:
///     TimeSpan dt = TimeSpan::FromMilliseconds(16.0);
///     TimeSpan t  = TimeSpan::FromSeconds(elapsed);
///
/// Reading goes through equally explicit accessors:
///     float ms = dt.MillisecondsF();
///     float s  = dt.SecondsF();
class TimeSpan
{
public:
	constexpr TimeSpan() : m_nMicros(0) {}

	static constexpr TimeSpan FromMicroseconds(int64_t us) { return TimeSpan(us); }
	static constexpr TimeSpan FromMilliseconds(double ms)  { return TimeSpan(static_cast<int64_t>(ms * 1000.0)); }
	static constexpr TimeSpan FromSeconds(double s)        { return TimeSpan(static_cast<int64_t>(s  * 1000000.0)); }
	static constexpr TimeSpan Zero()                       { return TimeSpan(0); }

	constexpr int64_t Microseconds()  const { return m_nMicros; }
	constexpr double  Milliseconds()  const { return static_cast<double>(m_nMicros) / 1000.0; }
	constexpr double  Seconds()       const { return static_cast<double>(m_nMicros) / 1000000.0; }
	constexpr float   MillisecondsF() const { return static_cast<float>(Milliseconds()); }
	constexpr float   SecondsF()      const { return static_cast<float>(Seconds()); }

	constexpr bool IsZero() const { return m_nMicros == 0; }

	constexpr TimeSpan& operator+=(TimeSpan rhs) { m_nMicros += rhs.m_nMicros; return *this; }
	constexpr TimeSpan& operator-=(TimeSpan rhs) { m_nMicros -= rhs.m_nMicros; return *this; }

	friend constexpr TimeSpan operator+(TimeSpan a, TimeSpan b) { return TimeSpan(a.m_nMicros + b.m_nMicros); }
	friend constexpr TimeSpan operator-(TimeSpan a, TimeSpan b) { return TimeSpan(a.m_nMicros - b.m_nMicros); }
	friend constexpr TimeSpan operator*(TimeSpan a, double k)   { return TimeSpan(static_cast<int64_t>(a.m_nMicros * k)); }
	friend constexpr TimeSpan operator*(double k, TimeSpan a)   { return a * k; }

	friend constexpr bool operator==(TimeSpan a, TimeSpan b) { return a.m_nMicros == b.m_nMicros; }
	friend constexpr bool operator!=(TimeSpan a, TimeSpan b) { return a.m_nMicros != b.m_nMicros; }
	friend constexpr bool operator< (TimeSpan a, TimeSpan b) { return a.m_nMicros <  b.m_nMicros; }
	friend constexpr bool operator<=(TimeSpan a, TimeSpan b) { return a.m_nMicros <= b.m_nMicros; }
	friend constexpr bool operator> (TimeSpan a, TimeSpan b) { return a.m_nMicros >  b.m_nMicros; }
	friend constexpr bool operator>=(TimeSpan a, TimeSpan b) { return a.m_nMicros >= b.m_nMicros; }

	static constexpr TimeSpan Min(TimeSpan a, TimeSpan b) { return (a < b) ? a : b; }
	static constexpr TimeSpan Max(TimeSpan a, TimeSpan b) { return (a > b) ? a : b; }

private:
	explicit constexpr TimeSpan(int64_t us) : m_nMicros(us) {}

	int64_t m_nMicros;
};

#endif
