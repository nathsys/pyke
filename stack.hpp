#pragma once

// Custom stack.
template <typename T>
struct Stack {
	T stack[64];
	Stack() : last(stack) {};
	T& go_next() { return (*last++); }
	void point_prev() { last--; }

private:
	T* last;
};
