function add64(a, b)
{
	var aLo = a[0] | 0, aHi = a[1] | 0;
	var bLo = b[0] | 0, bHi = b[1] | 0, carry;

	aLo = (aLo + bLo) | 0;
	carry = (aLo >>> 0 < bLo >>> 0) ? 1 : 0;
	aHi = (aHi + bHi + carry) | 0;
	return [aLo, aHi];
}

function mul32_64(a, b)
{
	var a0 = (a & 0xffff) | 0, a1 = (a >>> 16) | 0;
	var b0 = (b & 0xffff) | 0, b1 = (b >>> 16) | 0;
	var lo, mid0, mid1, hi, carry;

	lo   = (a0 * b0) | 0;
	mid0 = (a0 * b1) | 0;
	mid1 = (a1 * b0) | 0;
	hi   = (a1 * b1) | 0;
	mid0 = (mid0 + mid1) | 0;
	carry = (mid0 >>> 0 < mid1 >>> 0) ? 0x10000 | 0 : 0;
	hi = (hi + carry) | 0;

	mid1 = (mid0 >>> 16) | 0;
	mid0 = (mid0 <<  16) | 0;

	lo = (lo + mid0) | 0;
	carry = (lo >>> 0 < mid0 >>> 0) ? 1 : 0;
	hi = (hi + mid1 + carry) | 0;

	return [lo, hi];
}

function mul32(a, b)
{
	var a0 = (a & 0xffff) | 0, a1 = (a >>> 16) | 0;
	var b0 = (b & 0xffff) | 0, b1 = (b >>> 16) | 0;
	var lo, mid0, mid1;

	lo   =  (a0 * b0)        | 0;
	mid0 = ((a0 * b1) << 16) | 0;
	mid1 = ((a1 * b0) << 16) | 0;
	return (lo + mid0 + mid1) | 0;
}

function mul64(a, b)
{
	var ret;

	ret    =  mul32_64(a[0], b[0]);
	ret[1] = (mul32   (a[0], b[1]) + ret[1]) | 0;
	ret[1] = (mul32   (a[1], b[0]) + ret[1]) | 0;
	return ret;
}

function shiftRight64(a, shift)
{
	var ret;

	if (shift < 32)
	{
		ret = [
			((a[0] >>> shift) | (a[1] << (32 - shift))) | 0,
			 (a[1] >>> shift) | 0];
	}
	else
	{
		ret = [(a[1] >>> (shift - 32)) | 0, 0];
	}
	return ret;
}

function shiftLeft64(a, shift)
{
	var ret;

	if (shift < 32)
	{
		ret = [
			 (a[0] << shift) | 0,
			((a[1] << shift) | (a[0] >>> (32 - shift))) | 0];
	}
	else
	{
		ret = [(a[0] << (shift - 32)) | 0, 0];
	}
	return ret;
}

function xor64(a, b)
{
	return [(a[0] ^ b[0]) | 0, (a[1] ^ b[1]) | 0];
}

function xorshft64(state)
{
	var z;

	z = add64(state, [0x7f4a7c15, 0x9e3779b9]);
	state[0] = z[0];
	state[1] = z[1];

	z = mul64(xor64(z, shiftRight64(z, 30)), [0x1ce4e5b9, 0xbf58476d]);
	z = mul64(xor64(z, shiftRight64(z, 27)), [0x133111eb, 0x94d049bb]);
	return xor64(z, shiftRight64(z, 31));
}

function xorshft64_getState(z)
{
	z = mul64(xor64(xor64(z, shiftRight64(z, 31)), shiftRight64(z, 2*31)), [0xd24d8ec3, 0x319642b2]);
	z = mul64(xor64(xor64(z, shiftRight64(z, 27)), shiftRight64(z, 2*27)), [0x3f119089, 0x96de1b17]);
	return    xor64(xor64(z, shiftRight64(z, 30)), shiftRight64(z, 2*30));
}

function xorshft64_skip(state, count)
{
	var lo = state[0] | 0, hi = state[1] | 0, carry;
	var loConst = 0x7f4a7c15 | 0;
	var hiConst = 0x9e3779b9 | 0;

	if (count > 0)
	{
		for (var i = 0; i < count; i++) 
		{
			lo = (lo + loConst) | 0;
			carry = (lo >>> 0 < loConst >>> 0) ? 1 : 0;
			hi = (hi + hiConst + carry) | 0;
		}
	}
	else
	{
		for (var i = 0; i > count; i--)
		{
			carry = (lo >>> 0 < loConst >>> 0) ? 1 : 0;
			lo = (lo - loConst) | 0;
			hi = (hi - hiConst - carry) | 0;
		}
	}
	state[0] = lo;
	state[1] = hi;
}

function get64(arr, index)
{
	return [arr[2 * index] | 0, arr[2 * index + 1] | 0];
}

function isSrandom()
{
	var buffer = new Int32Array(2*(64+4));
	var x, y, z1, z2, z3, xorshft64_state, tmp0, tmp1;
	var err = 0;

	crypto.getRandomValues(buffer);

	z3 = xor64(xor64(get64(buffer, 1), get64(buffer, 64 + 0)), get64(buffer, 64 + 3));
	xorshft64_state = xorshft64_getState(z3);
	xorshft64_skip(xorshft64_state, -3);
	z1 = xorshft64(xorshft64_state);
	z2 = xorshft64(xorshft64_state);
	xorshft64_skip(xorshft64_state, 1);
	x = xor64(xor64(get64(buffer, 3), get64(buffer, 64 + 2)), z2);
	y = xor64(xor64(get64(buffer, 2), get64(buffer, 64 + 1)), z1);

	tmp0 = get64(buffer, 64 + 3);
	tmp1 = xor64(xor64(x, y), z3);
	if ((z1[0] & 1) != 0 || tmp0[0] != tmp1[0] || tmp0[1] != tmp1[1])
	{
		z1 = xor64(xor64(get64(buffer, 2), get64(buffer, 64 + 1)), get64(buffer, 64 + 3));
		xorshft64_state = xorshft64_getState(z1);
		z2 = xorshft64(xorshft64_state);
		z3 = xorshft64(xorshft64_state);
		x = xor64(xor64(get64(buffer, 1), get64(buffer, 64 + 0)), z2);
		y = xor64(xor64(get64(buffer, 3), get64(buffer, 64 + 2)), z3);

		tmp0 = get64(buffer, 64 + 3);
		tmp1 = xor64(xor64(x, y), z1);
		if ((z1[0] & 1) == 0 || tmp0[0] != tmp1[0] || tmp0[1] != tmp1[1])
		{
			err = 1;
		}
	}

	if (err)
	{
		x  = xor64(get64(buffer, 64 + 0), get64(buffer, 1));
		z1 = xor64(get64(buffer, 64 + 3), x);
		xorshft64_state = xorshft64_getState(z1);

		tmp0 = xorshft64(xorshft64_state);
		if ((z1[0] & 1) != 0 || tmp0[0] != x[0] || tmp0[1] != x[1])
		{
			x  = xor64(get64(buffer, 64 + 1), get64(buffer, 2));
			z1 = xor64(get64(buffer, 64 + 3), x);
			xorshft64_state = xorshft64_getState(z1);

			tmp0 = xorshft64(xorshft64_state);
			if ((z1[0] & 1) == 0 || tmp0[0] != x[0] || tmp0[1] != x[1])
			{
				return 0;
			}
		}
	}

	return 1;
}

// Each isSrandom() has a 1 in 2**62 chance of a false positive
// This is a 1 in 2**248 chance of a false positive
var badPrng = isSrandom() & isSrandom() & isSrandom() & isSrandom();

if (badPrng)
{
	alert("BAD: srandom PRNG detected");
}
else
{
	alert("GOOD: srandom PRNG *NOT* detected");
}
