using System; using NUnit.Framework; namespace Lox {

static class HashExtensions { // 64bit Fnv1a
    public const ulong SEED = 0xCBF29CE484222325;
    const ulong PRIME = 0x100000001B3;

    // 8bit => hash
    public static ulong Hash( this byte n, ulong chain = SEED ) => (chain ^ n) * PRIME;
    public static unsafe ulong Hash( this sbyte n, ulong chain = SEED ) => (chain ^ *(byte*)&n) * PRIME;
    public static unsafe ulong Hash( this bool n, ulong chain = SEED ) => (chain ^ *(byte*)&n) * PRIME;

    // 16bit => hash
    public static unsafe ulong Hash16( byte* p, ulong chain ) => (((chain ^ p[0]) * PRIME) ^ p[1]) * PRIME;
    public static unsafe ulong Hash( this ushort n, ulong chain = SEED ) => Hash16( (byte*)&n, chain );
    public static unsafe ulong Hash( this short n, ulong chain = SEED ) => Hash16( (byte*)&n, chain );
    public static unsafe ulong Hash( this char n, ulong chain = SEED ) => Hash16( (byte*)&n, chain );

    // 32bit => hash
    public static unsafe ulong Hash32( byte* p, ulong chain ) => (((((((chain ^ p[0]) * PRIME) ^ p[1]) * PRIME) ^ p[2]) * PRIME) ^ p[3]) * PRIME;
    public static unsafe ulong Hash( this uint n, ulong chain = SEED ) => Hash32( (byte*)&n, chain );
    public static unsafe ulong Hash( this int n, ulong chain = SEED ) => Hash32( (byte*)&n, chain );
    public static unsafe ulong Hash( this float n, ulong chain = SEED ) => Hash32( (byte*)&n, chain );

    // 64bit => hash
    public static unsafe ulong Hash64( byte* p, ulong chain ) => (((((((((((((((chain ^ p[0]) * PRIME) ^ p[1]) * PRIME) ^ p[2]) * PRIME) ^ p[3]) * PRIME) ^ p[4]) * PRIME) ^ p[5]) * PRIME) ^ p[6]) * PRIME) ^ p[7]) * PRIME;
    public static unsafe ulong Hash( this ulong n, ulong chain = SEED ) => Hash64( (byte*)&n, chain );
    public static unsafe ulong Hash( this double n, ulong chain = SEED ) => Hash64( (byte*)&n, chain );
    public static unsafe ulong Hash( this DateTime n, ulong chain = SEED ) => Hash64( (byte*)&n, chain );
    public static unsafe ulong Hash( this TimeSpan n, ulong chain = SEED ) => Hash64( (byte*)&n, chain );

    // Nbit hash => (N/2)bit hash
    public static unsafe uint Xorfold( this ulong n ) => (uint)(n ^ (n >> 32));
    public static unsafe ushort Xorfold( this uint n ) => (ushort)(n ^ (n >> 16));
    public static unsafe byte Xorfold( this ushort n ) => (byte)(n ^ (n >> 8));

    // 8bit reinterpret cast
    public static unsafe sbyte AsSByte( this byte n ) => *(sbyte*)&n;
    public static unsafe byte AsByte( this sbyte n ) => *(byte*)&n;
    
    // 16bit reinterpret cast
    public static unsafe short AsShort( this ushort n ) => *(short*)&n;
    public static unsafe short AsShort( this Half n ) => *(short*)&n;
    public static unsafe short AsShort( this char n ) => *(short*)&n;
    public static unsafe ushort AsUShort( this short n ) => *(ushort*)&n;
    public static unsafe ushort AsUShort( this Half n ) => *(ushort*)&n;
    public static unsafe ushort AsUShort( this char n ) => *(ushort*)&n;
    public static unsafe Half AsHalf( this short n ) => *(Half*)&n;
    public static unsafe Half AsHalf( this ushort n ) => *(Half*)&n;
    public static unsafe Half AsHalf( this char n ) => *(Half*)&n;
    
    // 32bit reinterpret cast
    public static unsafe int AsInt( this uint n ) => *(int*)&n;
    public static unsafe int AsInt( this float n ) => *(int*)&n;
    public static unsafe uint AsUInt( this int n ) => *(uint*)&n;
    public static unsafe uint AsUInt( this float n ) => *(uint*)&n;
    public static unsafe float AsFloat( this int n ) => *(float*)&n;
    public static unsafe float AsFloat( this uint n ) => *(float*)&n;    

    // 64bit reinterpret cast
    public static unsafe long AsLong( this ulong n ) => *(long*)&n;
    public static unsafe long AsLong( this double n ) => *(long*)&n;
    public static unsafe ulong AsULong( this long n ) => *(ulong*)&n;
    public static unsafe ulong AsULong( this double n ) => *(ulong*)&n;
    public static unsafe double AsDouble( this long n ) => *(double*)&n;
    public static unsafe double AsDouble( this ulong n ) => *(double*)&n;

    // tests
    [Test] public static void Fnv1a() { // http://isthe.com/chongo/src/fnv/test_fnv.c
        Assert.AreEqual( 0xaf63dc4c8601ec8c, ((byte)97).Hash() ); // ASCII 'a'
        Assert.AreEqual( 0xaf63db4c8601ead9, ((byte)102).Hash() ); // ASCII 'f'
        Assert.AreEqual( 0xdd120e790c2512af, ((98 << 24) | (111 << 16) | (111 << 8) | 102).Hash() ); // ASCII 'foob'
    }
}

} // namespace
