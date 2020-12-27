#include <SDL.h>
#include "types.h"
#include "utility.h"

MemoryBuffer ReadBinaryFile(const char *path)
{
	auto ctx = SDL_RWFromFile(path, "rb");
	auto file_size = ctx->size(ctx);
	MemoryBuffer buffer = { new uint8_t[file_size], file_size };
	ctx->read(ctx, buffer.data, file_size, 1);
	ctx->close(ctx);
	return buffer;
}
