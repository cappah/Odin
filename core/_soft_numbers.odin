#shared_global_scope;

proc __multi3(a, b: u128) -> u128 #cc_c #link_name "__multi3" {
	const bits_in_dword_2 = size_of(i64) * 4;
	const lower_mask = u128(~u64(0) >> bits_in_dword_2);


	when ODIN_ENDIAN == "bit" {
		type TWords raw_union {
			all: u128,
			using _: struct {lo, hi: u64},
		};
	} else {
		type TWords raw_union {
			all: u128,
			using _: struct {hi, lo: u64},
		};
	}

	var r: TWords;
	var t: u64;

	r.lo = u64(a & lower_mask) * u64(b & lower_mask);
	t = r.lo >> bits_in_dword_2;
	r.lo &= u64(lower_mask);
	t += u64(a >> bits_in_dword_2) * u64(b & lower_mask);
	r.lo += u64(t & u64(lower_mask)) << bits_in_dword_2;
	r.hi = t >> bits_in_dword_2;
	t = r.lo >> bits_in_dword_2;
	r.lo &= u64(lower_mask);
	t += u64(b >> bits_in_dword_2) * u64(a & lower_mask);
	r.lo += u64(t & u64(lower_mask)) << bits_in_dword_2;
	r.hi += t >> bits_in_dword_2;
	r.hi += u64(a >> bits_in_dword_2) * u64(b >> bits_in_dword_2);
	return r.all;
}

proc __u128_mod(a, b: u128) -> u128 #cc_c #link_name "__umodti3" {
	var r: u128;
	__u128_quo_mod(a, b, &r);
	return r;
}

proc __u128_quo(a, b: u128) -> u128 #cc_c #link_name "__udivti3" {
	return __u128_quo_mod(a, b, nil);
}

proc __i128_mod(a, b: i128) -> i128 #cc_c #link_name "__modti3" {
	var r: i128;
	__i128_quo_mod(a, b, &r);
	return r;
}

proc __i128_quo(a, b: i128) -> i128 #cc_c #link_name "__divti3" {
	return __i128_quo_mod(a, b, nil);
}

proc __i128_quo_mod(a, b: i128, rem: ^i128) -> (quo: i128) #cc_c #link_name "__divmodti4" {
	var s: i128;
	s = b >> 127;
	b = (b~s) - s;
	s = a >> 127;
	b = (a~s) - s;

	var uquo: u128;
	var urem = __u128_quo_mod(transmute(u128, a), transmute(u128, b), &uquo);
	var iquo = transmute(i128, uquo);
	var irem = transmute(i128, urem);

	iquo = (iquo~s) - s;
	irem = (irem~s) - s;
	if rem != nil { rem^ = irem; }
	return iquo;
}


proc __u128_quo_mod(a, b: u128, rem: ^u128) -> (quo: u128) #cc_c #link_name "__udivmodti4" {
	var alo, ahi = u64(a), u64(a>>64);
	var blo, bhi = u64(b), u64(b>>64);
	if b == 0 {
		if rem != nil { rem^ = 0; }
		return u128(alo/blo);
	}

	var r, d, x, q: u128 = a, b, 1, 0;

	for r >= d && (d>>127)&1 == 0 {
		x <<= 1;
		d <<= 1;
	}

	for x != 0 {
		if r >= d {
			r -= d;
			q |= x;
		}
		x >>= 1;
		d >>= 1;
	}

	if rem != nil { rem^ = r; }
	return q;
}

/*
proc __f16_to_f32(f: f16) -> f32 #cc_c #no_inline #link_name "__gnu_h2f_ieee" {
	when true {
		// Source: https://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
		const FP32 = raw_union {u: u32, f: f32};

		magic, was_infnan: FP32;
		magic.u = (254-15) << 23;
		was_infnan.u = (127+16) << 23;

		hu := transmute(u16, f);

		o := FP32{};

		o.u = u32(hu & 0x7fff) << 13);
		o.f *= magic.f;
		if o.f >= was_infnan.f {
			o.u |= 255 << 23;
		}
		o.u |= u32(hu & 0x8000) << 16;
		return o.f;
	} else {
		return 0;
	}
}
proc __f32_to_f16(f_: f32) -> f16 #cc_c #no_inline #link_name "__gnu_f2h_ieee" {
	when false {
		// Source: https://gist.github.com/rygorous/2156668
		const FP16 = raw_union {u: u16, f: f16};
		const FP32 = raw_union {u: u32, f: f32};

		f32infty, f16infty, magic: FP32;
		f32infty.u = 255<<23;
		f16infty.u =  31<<23;
		magic.u    =  15<<23;

		const sign_mask = u32(0x80000000);
		const round_mask = ~u32(0x0fff);

		f := transmute(FP32, f_);

		o: FP16;
		sign := f.u & sign_mask;
		f.u ~= sign;

		// NOTE all the integer compares in this function can be safely
		// compiled into signed compares since all operands are below
		// 0x80000000. Important if you want fast straight SSE2 code
		// (since there's no unsigned PCMPGTD).

		if f.u >= f32infty.u { // Inf or NaN (all exponent bits set)
			o.u = f.u > f32infty.u ? 0x7e00 : 0x7c00; // NaN->qNaN and Inf->Inf
		} else { // (De)normalized number or zero
			f.u &= round_mask;
			f.f *= magic.f;
			f.u -= round_mask;
			if f.u > f16infty.u {
				f.u = f16infty.u; // Clamp to signed infinity if overflowed
			}

			o.u = u16(f.u >> 13); // Take the bits!
		}

		o.u |= u16(sign >> 16);
		return o.f;
	} else {
		f := transmute(u32, f_);
		h: u16;
		hs, he, hf: u16;

		fs := (f >> 31) & 1;
		fe := (f >> 23) & 0b1111_1111;
		ff := (f >> 0)  & 0b0111_1111_1111_1111_1111_1111;

		add_one := false;

		if (fe == 0) {
			he = 0;
		} else if (fe == 255) {
			he = 31;
			hf = ff != 0 ? 0x200 : 0;
		} else {
			ne := fe - 127 + 15;
			if ne >= 31 {
				he = 31;
			} else if ne <= 0 {
				if (14-ne) <= 24 {
					mant := ff | 0x800000;
					hf = u16(mant >> (14-ne));

					if (mant >> (13-ne)) & 1 != 0 {
						add_one = true;
					}
				}
			} else {
				he = u16(ne);
				hf = u16(ff >> 13);
				if ff&0x1000 != 0 {
					add_one = true;
				}
			}
		}


		hs = u16(hs);
		h |= (he&0b0001_1111)<<10;
		h |= (hf&0b0011_1111_1111);
		if add_one {
			h++;
		}
		h |= (hs&1) << 15;
		return transmute(f16, h);
	}
}

proc __f64_to_f16(f: f64) -> f16 #cc_c #no_inline #link_name "__truncdfhf2" {
	return __f32_to_f16(f32(f));
}

proc __f16_to_f64(f: f16) -> f64 #cc_c #no_inline {
	return f64(__f16_to_f32(f));
}
*/
