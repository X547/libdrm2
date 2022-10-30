#pragma once

#include <algorithm>


template<typename F>
class ScopeExit 
{
public:
	explicit ScopeExit(F&& fn) : fFn(fn) {}
	ScopeExit(ScopeExit&& other) : fFn(std::move(other.fFn)) {}
	~ScopeExit(){fFn();}
private:
	ScopeExit(const ScopeExit&);
	ScopeExit& operator=(const ScopeExit&);
private:
	F fFn;
};

template<typename F>
ScopeExit<F> MakeScopeExit(F&& fn)
{
	return ScopeExit<F>(std::move(fn));
}
