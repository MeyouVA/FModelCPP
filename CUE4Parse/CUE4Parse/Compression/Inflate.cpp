// A compact canonical-Huffman DEFLATE inflater (RFC 1951) with zlib/gzip wrappers.
// Structure follows Mark Adler's public-domain reference decoder "puff"; adapted to write into a fixed
// output buffer and to report errors by return code rather than setjmp/longjmp.
#include "Inflate.h"

namespace CUE4Parse::Compression
{
    namespace
    {
        constexpr int MAXBITS = 15;    // maximum bits in a code
        constexpr int MAXLCODES = 286; // maximum number of literal/length codes
        constexpr int MAXDCODES = 30;  // maximum number of distance codes
        constexpr int MAXCODES = MAXLCODES + MAXDCODES;
        constexpr int FIXLCODES = 288; // number of fixed literal/length codes

        struct State
        {
            uint8_t* out;
            long outlen;
            long outcnt;
            const uint8_t* in;
            long inlen;
            long incnt;
            int bitbuf;
            int bitcnt;
            bool err; // set when input is exhausted mid-read
        };

        struct Huffman
        {
            short* count;  // number of symbols of each length
            short* symbol; // canonically ordered symbols
        };

        // Return need bits from the stream, LSB-first. Sets err (returns 0) if input runs out.
        int Bits(State& s, int need)
        {
            long val = s.bitbuf;
            while (s.bitcnt < need)
            {
                if (s.incnt == s.inlen) { s.err = true; return 0; }
                val |= static_cast<long>(s.in[s.incnt++]) << s.bitcnt;
                s.bitcnt += 8;
            }
            s.bitbuf = static_cast<int>(val >> need);
            s.bitcnt -= need;
            return static_cast<int>(val & ((1L << need) - 1));
        }

        // Decode one symbol using the given canonical-Huffman table. Returns the symbol, or a negative error.
        int Decode(State& s, const Huffman& h)
        {
            int code = 0, first = 0, index = 0;
            for (int len = 1; len <= MAXBITS; len++)
            {
                code |= Bits(s, 1);
                if (s.err) return -11;
                const int count = h.count[len];
                if (code - count < first) // length len, this many codes
                    return h.symbol[index + (code - first)];
                index += count;
                first += count;
                first <<= 1;
                code <<= 1;
            }
            return -10; // ran out of codes
        }

        // Build a canonical-Huffman decode table from a list of code lengths.
        // Returns 0 for a complete code set, negative if over-subscribed, positive (incomplete) otherwise.
        int Construct(Huffman& h, const short* length, int n)
        {
            for (int len = 0; len <= MAXBITS; len++)
                h.count[len] = 0;
            for (int symbol = 0; symbol < n; symbol++)
                h.count[length[symbol]]++;
            if (h.count[0] == n) // no codes at all -> complete but empty
                return 0;

            int left = 1; // one possible code of zero length
            for (int len = 1; len <= MAXBITS; len++)
            {
                left <<= 1;                // one more bit, double codes
                left -= h.count[len];      // deduct used codes
                if (left < 0) return left; // over-subscribed
            }

            short offs[MAXBITS + 1];
            offs[1] = 0;
            for (int len = 1; len < MAXBITS; len++)
                offs[len + 1] = static_cast<short>(offs[len] + h.count[len]);
            for (int symbol = 0; symbol < n; symbol++)
                if (length[symbol] != 0)
                    h.symbol[offs[length[symbol]]++] = static_cast<short>(symbol);

            return left; // 0 complete, >0 incomplete
        }

        // Decode literal/length + distance codes into the output buffer.
        int Codes(State& s, const Huffman& lencode, const Huffman& distcode)
        {
            static const short lens[29] = {
                3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
            };
            static const short lext[29] = {
                0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
            };
            static const short dists[30] = {
                1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
                257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
            };
            static const short dext[30] = {
                0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
                7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
            };

            int symbol;
            do
            {
                symbol = Decode(s, lencode);
                if (symbol < 0) return symbol;

                if (symbol < 256) // literal byte
                {
                    if (s.outcnt == s.outlen) return -1; // output overrun
                    s.out[s.outcnt++] = static_cast<uint8_t>(symbol);
                }
                else if (symbol > 256) // length
                {
                    symbol -= 257;
                    if (symbol >= 29) return -10; // invalid length symbol
                    int len = lens[symbol] + Bits(s, lext[symbol]);
                    if (s.err) return -11;

                    symbol = Decode(s, distcode);
                    if (symbol < 0) return symbol;
                    const int dist = dists[symbol] + Bits(s, dext[symbol]);
                    if (s.err) return -11;
                    if (dist > s.outcnt) return -12; // distance too far back

                    if (s.outcnt + len > s.outlen) return -1; // output overrun
                    while (len--)
                    {
                        s.out[s.outcnt] = s.out[s.outcnt - dist];
                        s.outcnt++;
                    }
                }
            } while (symbol != 256); // end-of-block

            return 0;
        }

        int Stored(State& s)
        {
            // Discard any bits remaining in the current byte.
            s.bitbuf = 0;
            s.bitcnt = 0;

            if (s.incnt + 4 > s.inlen) return -2;
            const int len = s.in[s.incnt] | (s.in[s.incnt + 1] << 8);
            const int nlen = s.in[s.incnt + 2] | (s.in[s.incnt + 3] << 8);
            s.incnt += 4;
            if ((len ^ 0xffff) != nlen) return -3; // LEN/NLEN mismatch

            if (s.incnt + len > s.inlen) return -2;
            if (s.outcnt + len > s.outlen) return -1;
            for (int i = 0; i < len; i++)
                s.out[s.outcnt++] = s.in[s.incnt++];
            return 0;
        }

        int Fixed(State& s)
        {
            static bool built = false;
            static short lencnt[MAXBITS + 1], lensym[FIXLCODES];
            static short distcnt[MAXBITS + 1], distsym[MAXDCODES];
            static Huffman lencode{lencnt, lensym};
            static Huffman distcode{distcnt, distsym};

            if (!built)
            {
                short lengths[FIXLCODES];
                int symbol = 0;
                for (; symbol < 144; symbol++) lengths[symbol] = 8;
                for (; symbol < 256; symbol++) lengths[symbol] = 9;
                for (; symbol < 280; symbol++) lengths[symbol] = 7;
                for (; symbol < FIXLCODES; symbol++) lengths[symbol] = 8;
                Construct(lencode, lengths, FIXLCODES);

                for (symbol = 0; symbol < MAXDCODES; symbol++) lengths[symbol] = 5;
                Construct(distcode, lengths, MAXDCODES);
                built = true;
            }

            return Codes(s, lencode, distcode);
        }

        int Dynamic(State& s)
        {
            static const short order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

            short lengths[MAXCODES];
            short lencnt[MAXBITS + 1], lensym[MAXLCODES];
            short distcnt[MAXBITS + 1], distsym[MAXDCODES];
            Huffman lencode{lencnt, lensym};
            Huffman distcode{distcnt, distsym};

            const int nlen = Bits(s, 5) + 257;
            const int ndist = Bits(s, 5) + 1;
            const int ncode = Bits(s, 4) + 4;
            if (s.err) return -11;
            if (nlen > MAXLCODES || ndist > MAXDCODES) return -3;

            int index;
            for (index = 0; index < ncode; index++)
            {
                lengths[order[index]] = static_cast<short>(Bits(s, 3));
                if (s.err) return -11;
            }
            for (; index < 19; index++)
                lengths[order[index]] = 0;

            int err = Construct(lencode, lengths, 19);
            if (err != 0) return -4; // code-length code must be complete

            index = 0;
            while (index < nlen + ndist)
            {
                int symbol = Decode(s, lencode);
                if (symbol < 0) return symbol;

                if (symbol < 16) // literal length 0..15
                {
                    lengths[index++] = static_cast<short>(symbol);
                }
                else
                {
                    int len = 0;
                    if (symbol == 16) // repeat previous length 3..6 times
                    {
                        if (index == 0) return -5; // no previous length
                        len = lengths[index - 1];
                        symbol = 3 + Bits(s, 2);
                    }
                    else if (symbol == 17) // repeat zero 3..10 times
                    {
                        symbol = 3 + Bits(s, 3);
                    }
                    else // == 18: repeat zero 11..138 times
                    {
                        symbol = 11 + Bits(s, 7);
                    }
                    if (s.err) return -11;
                    if (index + symbol > nlen + ndist) return -6; // too many lengths
                    while (symbol--)
                        lengths[index++] = static_cast<short>(len);
                }
            }

            if (lengths[256] == 0) return -9; // no end-of-block code

            err = Construct(lencode, lengths, nlen);
            // Incomplete lit/length codes are only allowed when there is a single such code.
            if (err && (err < 0 || nlen != lencode.count[0] + lencode.count[1])) return -7;

            err = Construct(distcode, lengths + nlen, ndist);
            if (err && (err < 0 || ndist != distcode.count[0] + distcode.count[1])) return -8;

            return Codes(s, lencode, distcode);
        }
    }

    int Inflate(const uint8_t* src, int srcLen, uint8_t* dst, int dstCap)
    {
        State s;
        s.out = dst;
        s.outlen = dstCap;
        s.outcnt = 0;
        s.in = src;
        s.inlen = srcLen;
        s.incnt = 0;
        s.bitbuf = 0;
        s.bitcnt = 0;
        s.err = false;

        int err;
        int last;
        do
        {
            last = Bits(s, 1);
            const int type = Bits(s, 2);
            if (s.err) return -11;

            if (type == 0) err = Stored(s);
            else if (type == 1) err = Fixed(s);
            else if (type == 2) err = Dynamic(s);
            else return -13; // reserved block type

            if (err != 0) return err;
        } while (!last);

        return static_cast<int>(s.outcnt);
    }

    int ZlibDecompress(const uint8_t* src, int srcLen, uint8_t* dst, int dstCap)
    {
        if (srcLen < 2) return -20;
        const int cmf = src[0];
        const int flg = src[1];
        if ((cmf & 0x0f) != 8) return -21;                  // compression method must be DEFLATE
        if (((cmf << 8) | flg) % 31 != 0) return -22;       // header checksum
        int offset = 2;
        if (flg & 0x20) offset += 4;                        // FDICT: skip 4-byte preset dictionary id
        if (offset > srcLen) return -20;
        return Inflate(src + offset, srcLen - offset, dst, dstCap);
    }

    int GzipDecompress(const uint8_t* src, int srcLen, uint8_t* dst, int dstCap)
    {
        if (srcLen < 18) return -30;                        // 10-byte header + at least 8-byte trailer
        if (src[0] != 0x1f || src[1] != 0x8b) return -31;   // magic
        if (src[2] != 8) return -32;                        // DEFLATE
        const int flg = src[3];
        int offset = 10;                                    // magic, method, flags, mtime(4), xfl, os

        if (flg & 0x04) // FEXTRA
        {
            if (offset + 2 > srcLen) return -30;
            const int xlen = src[offset] | (src[offset + 1] << 8);
            offset += 2 + xlen;
        }
        if (flg & 0x08) // FNAME: zero-terminated
        {
            while (offset < srcLen && src[offset] != 0) offset++;
            offset++;
        }
        if (flg & 0x10) // FCOMMENT: zero-terminated
        {
            while (offset < srcLen && src[offset] != 0) offset++;
            offset++;
        }
        if (flg & 0x02) offset += 2; // FHCRC

        if (offset > srcLen) return -30;
        return Inflate(src + offset, srcLen - offset, dst, dstCap);
    }
}
