#pragma once

#define ALLOCATE(type, count) \
	(type*)reallocate(nullptr, 0, sizeof(type) * (count))

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

void freeObjects();