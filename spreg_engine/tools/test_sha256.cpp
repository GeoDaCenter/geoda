// Test vectors for Regression/SpregSha256.h - the digest GeoDa checks every
// engine download against, so it had better be exactly right.
//
//     c++ -std=gnu++14 -O2 -I../.. -o /tmp/test_sha256 test_sha256.cpp && /tmp/test_sha256
//
// The last two cases are checked against the system's own sha256 as well, which
// is what the shell one-liner in the build/test instructions does.

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "../../Regression/SpregSha256.h"

using SpregEngine::Sha256;

static int failures = 0;

static void check(const std::string& what, const std::string& got,
				  const std::string& want)
{
	const bool ok = (got == want);
	if (!ok) ++failures;
	std::printf("%-46s %s\n", what.c_str(), ok ? "ok" : "FAIL");
	if (!ok) {
		std::printf("   got  %s\n   want %s\n", got.c_str(), want.c_str());
	}
}

static std::string hash_of(const std::string& data)
{
	Sha256 h;
	h.Update(data.data(), data.size());
	return h.HexDigest();
}

int main()
{
	// FIPS 180-4 / RFC 6234 vectors
	check("empty", hash_of(""),
		  "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
	check("abc", hash_of("abc"),
		  "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	check("448-bit message", hash_of("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
		  "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
	check("896-bit message",
		  hash_of("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
				  "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"),
		  "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");

	// one million 'a', in one shot and in awkward chunks
	{
		const std::string million(1000000, 'a');
		check("1000000 x 'a' (one shot)", hash_of(million),
			  "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");

		Sha256 h;
		const size_t chunks[] = {1, 63, 64, 65, 1000, 4096, 12345};
		size_t done = 0;
		int i = 0;
		while (done < million.size()) {
			size_t take = chunks[i++ % 7];
			if (take > million.size() - done) take = million.size() - done;
			h.Update(million.data() + done, take);
			done += take;
		}
		check("1000000 x 'a' (7 chunk sizes)", h.HexDigest(),
			  "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
	}

	// something bigger than any single block, exercising many rounds, against
	// a file the caller can hash independently
	{
		std::vector<char> data;
		data.reserve(300000);
		unsigned int x = 123456789u;
		for (int i = 0; i < 300000; ++i) {
			x = x * 1103515245u + 12345u;          // deterministic, not random
			data.push_back(static_cast<char>((x >> 16) & 0xff));
		}
		const std::string path = "/tmp/geoda_sha256_test.bin";
		std::ofstream out(path.c_str(), std::ios::binary);
		out.write(&data[0], static_cast<std::streamsize>(data.size()));
		out.close();

		Sha256 h;
		h.Update(&data[0], data.size());
		std::printf("%-46s %s\n", "300000 pseudo-random bytes", h.HexDigest().c_str());
		std::printf("   (compare with: shasum -a 256 %s)\n", path.c_str());
	}

	std::printf("\n%s\n", failures ? "FAILED" : "all vectors passed");
	return failures ? 1 : 0;
}
