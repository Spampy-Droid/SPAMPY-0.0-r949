/* Native CZ-1 integration; native DSP by Kerem Kilic / Devin Acker. */
#include "cz_patch.h"
#include "cz_selected.h"
static const float FM6_SIN[1025]={0x0.0p+0f,0x1.921f000000000p-8f,0x1.921d000000000p-7f,0x1.2d93800000000p-6f,0x1.9215800000000p-6f,0x1.f693800000000p-6f,0x1.2d86600000000p-5f,0x1.5fc0000000000p-5f,0x1.91f6600000000p-5f,0x1.c428e00000000p-5f,0x1.f656e00000000p-5f,0x1.1440100000000p-4f,0x1.2d52100000000p-4f,0x1.4661200000000p-4f,0x1.5f6d000000000p-4f,0x1.7875800000000p-4f,0x1.917a700000000p-4f,0x1.aa7b700000000p-4f,0x1.c378600000000p-4f,0x1.dc70f00000000p-4f,0x1.f564e00000000p-4f,0x1.072a080000000p-3f,0x1.139f100000000p-3f,0x1.2011700000000p-3f,0x1.2c81080000000p-3f,0x1.38edb80000000p-3f,0x1.4557680000000p-3f,0x1.51bdf80000000p-3f,0x1.5e21480000000p-3f,0x1.6a81300000000p-3f,0x1.76dda00000000p-3f,0x1.8336700000000p-3f,0x1.8f8b800000000p-3f,0x1.9bdcc00000000p-3f,0x1.a82a000000000p-3f,0x1.b473300000000p-3f,0x1.c0b8280000000p-3f,0x1.ccf8c80000000p-3f,0x1.d935000000000p-3f,0x1.e56ca00000000p-3f,0x1.f19f980000000p-3f,0x1.fdcdc00000000p-3f,0x1.04fb800000000p-2f,0x1.0b0d9c0000000p-2f,0x1.111d280000000p-2f,0x1.172a0c0000000p-2f,0x1.1d34440000000p-2f,0x1.233bbc0000000p-2f,0x1.2940640000000p-2f,0x1.2f422c0000000p-2f,0x1.35410c0000000p-2f,0x1.3b3cf00000000p-2f,0x1.4135c80000000p-2f,0x1.472b8c0000000p-2f,0x1.4d1e240000000p-2f,0x1.530d880000000p-2f,0x1.58f9a80000000p-2f,0x1.5ee2740000000p-2f,0x1.64c7dc0000000p-2f,0x1.6aa9d80000000p-2f,0x1.7088540000000p-2f,0x1.7663400000000p-2f,0x1.7c3a940000000p-2f,0x1.820e3c0000000p-2f,0x1.87de2c0000000p-2f,0x1.8daa540000000p-2f,0x1.9372a80000000p-2f,0x1.9937180000000p-2f,0x1.9ef7940000000p-2f,0x1.a4b4140000000p-2f,0x1.aa6c840000000p-2f,0x1.b020d80000000p-2f,0x1.b5d1000000000p-2f,0x1.bb7cf40000000p-2f,0x1.c1249c0000000p-2f,0x1.c6c7f40000000p-2f,0x1.cc66e80000000p-2f,0x1.d201700000000p-2f,0x1.d797740000000p-2f,0x1.dd28f00000000p-2f,0x1.e2b5d40000000p-2f,0x1.e83e100000000p-2f,0x1.edc1940000000p-2f,0x1.f340580000000p-2f,0x1.f8ba4c0000000p-2f,0x1.fe2f640000000p-2f,0x1.01cfc80000000p-1f,0x1.0485620000000p-1f,0x1.07387a0000000p-1f,0x1.09e9080000000p-1f,0x1.0c97040000000p-1f,0x1.0f426c0000000p-1f,0x1.11eb360000000p-1f,0x1.14915a0000000p-1f,0x1.1734d60000000p-1f,0x1.19d5a00000000p-1f,0x1.1c73b40000000p-1f,0x1.1f0f080000000p-1f,0x1.21a79a0000000p-1f,0x1.243d600000000p-1f,0x1.26d0540000000p-1f,0x1.2960720000000p-1f,0x1.2bedb20000000p-1f,0x1.2e780e0000000p-1f,0x1.30ff800000000p-1f,0x1.3384000000000p-1f,0x1.36058a0000000p-1f,0x1.3884180000000p-1f,0x1.3affa20000000p-1f,0x1.3d78240000000p-1f,0x1.3fed960000000p-1f,0x1.425ff20000000p-1f,0x1.44cf320000000p-1f,0x1.473b520000000p-1f,0x1.49a44a0000000p-1f,0x1.4c0a140000000p-1f,0x1.4e6cac0000000p-1f,0x1.50cc0a0000000p-1f,0x1.53282a0000000p-1f,0x1.5581040000000p-1f,0x1.57d6940000000p-1f,0x1.5a28d20000000p-1f,0x1.5c77bc0000000p-1f,0x1.5ec34a0000000p-1f,0x1.610b760000000p-1f,0x1.63503a0000000p-1f,0x1.6591920000000p-1f,0x1.67cf780000000p-1f,0x1.6a09e60000000p-1f,0x1.6c40d80000000p-1f,0x1.6e74460000000p-1f,0x1.70a42c0000000p-1f,0x1.72d0840000000p-1f,0x1.74f9480000000p-1f,0x1.771e760000000p-1f,0x1.7940060000000p-1f,0x1.7b5df20000000p-1f,0x1.7d78360000000p-1f,0x1.7f8ece0000000p-1f,0x1.81a1b40000000p-1f,0x1.83b0e00000000p-1f,0x1.85bc520000000p-1f,0x1.87c4000000000p-1f,0x1.89c7ea0000000p-1f,0x1.8bc8060000000p-1f,0x1.8dc4520000000p-1f,0x1.8fbcca0000000p-1f,0x1.91b1660000000p-1f,0x1.93a2240000000p-1f,0x1.958efe0000000p-1f,0x1.9777f00000000p-1f,0x1.995cf20000000p-1f,0x1.9b3e040000000p-1f,0x1.9d1b200000000p-1f,0x1.9ef43e0000000p-1f,0x1.a0c95e0000000p-1f,0x1.a29a7a0000000p-1f,0x1.a4678c0000000p-1f,0x1.a630920000000p-1f,0x1.a7f5840000000p-1f,0x1.a9b6620000000p-1f,0x1.ab73260000000p-1f,0x1.ad2bca0000000p-1f,0x1.aee04a0000000p-1f,0x1.b090a60000000p-1f,0x1.b23cd40000000p-1f,0x1.b3e4d40000000p-1f,0x1.b588a00000000p-1f,0x1.b728340000000p-1f,0x1.b8c38c0000000p-1f,0x1.ba5aa60000000p-1f,0x1.bbed7c0000000p-1f,0x1.bd7c0a0000000p-1f,0x1.bf064e0000000p-1f,0x1.c08c420000000p-1f,0x1.c20de40000000p-1f,0x1.c38b2e0000000p-1f,0x1.c504200000000p-1f,0x1.c678b20000000p-1f,0x1.c7e8e40000000p-1f,0x1.c954b20000000p-1f,0x1.cabc160000000p-1f,0x1.cc1f0e0000000p-1f,0x1.cd7d980000000p-1f,0x1.ced7ae0000000p-1f,0x1.d02d500000000p-1f,0x1.d17e760000000p-1f,0x1.d2cb220000000p-1f,0x1.d4134c0000000p-1f,0x1.d556f40000000p-1f,0x1.d696160000000p-1f,0x1.d7d0b00000000p-1f,0x1.d906bc0000000p-1f,0x1.da383a0000000p-1f,0x1.db65260000000p-1f,0x1.dc8d7c0000000p-1f,0x1.ddb13c0000000p-1f,0x1.ded0600000000p-1f,0x1.dfeae60000000p-1f,0x1.e100cc0000000p-1f,0x1.e212100000000p-1f,0x1.e31eae0000000p-1f,0x1.e426a40000000p-1f,0x1.e529f00000000p-1f,0x1.e6288e0000000p-1f,0x1.e7227e0000000p-1f,0x1.e817ba0000000p-1f,0x1.e908420000000p-1f,0x1.e9f4140000000p-1f,0x1.eadb2e0000000p-1f,0x1.ebbd8c0000000p-1f,0x1.ec9b2c0000000p-1f,0x1.ed740e0000000p-1f,0x1.ee482e0000000p-1f,0x1.ef178a0000000p-1f,0x1.efe2200000000p-1f,0x1.f0a7f00000000p-1f,0x1.f168f40000000p-1f,0x1.f2252e0000000p-1f,0x1.f2dc9c0000000p-1f,0x1.f38f3a0000000p-1f,0x1.f43d080000000p-1f,0x1.f4e6040000000p-1f,0x1.f58a2a0000000p-1f,0x1.f6297c0000000p-1f,0x1.f6c3f80000000p-1f,0x1.f7599a0000000p-1f,0x1.f7ea620000000p-1f,0x1.f876500000000p-1f,0x1.f8fd600000000p-1f,0x1.f97f920000000p-1f,0x1.f9fce40000000p-1f,0x1.fa75580000000p-1f,0x1.fae8e80000000p-1f,0x1.fb57960000000p-1f,0x1.fbc1600000000p-1f,0x1.fc26460000000p-1f,0x1.fc86460000000p-1f,0x1.fce1600000000p-1f,0x1.fd37900000000p-1f,0x1.fd88da0000000p-1f,0x1.fdd53a0000000p-1f,0x1.fe1cb00000000p-1f,0x1.fe5f3a0000000p-1f,0x1.fe9cda0000000p-1f,0x1.fed58e0000000p-1f,0x1.ff09560000000p-1f,0x1.ff38300000000p-1f,0x1.ff621e0000000p-1f,0x1.ff871c0000000p-1f,0x1.ffa72e0000000p-1f,0x1.ffc2520000000p-1f,0x1.ffd8860000000p-1f,0x1.ffe9ca0000000p-1f,0x1.fff6200000000p-1f,0x1.fffd880000000p-1f,0x1.0000000000000p+0f,0x1.fffd880000000p-1f,0x1.fff6200000000p-1f,0x1.ffe9ca0000000p-1f,0x1.ffd8860000000p-1f,0x1.ffc2520000000p-1f,0x1.ffa72e0000000p-1f,0x1.ff871c0000000p-1f,0x1.ff621e0000000p-1f,0x1.ff38300000000p-1f,0x1.ff09560000000p-1f,0x1.fed58e0000000p-1f,0x1.fe9cda0000000p-1f,0x1.fe5f3a0000000p-1f,0x1.fe1cae0000000p-1f,0x1.fdd53a0000000p-1f,0x1.fd88da0000000p-1f,0x1.fd37900000000p-1f,0x1.fce15e0000000p-1f,0x1.fc86460000000p-1f,0x1.fc26460000000p-1f,0x1.fbc1600000000p-1f,0x1.fb57960000000p-1f,0x1.fae8e80000000p-1f,0x1.fa75580000000p-1f,0x1.f9fce40000000p-1f,0x1.f97f920000000p-1f,0x1.f8fd600000000p-1f,0x1.f8764e0000000p-1f,0x1.f7ea620000000p-1f,0x1.f7599a0000000p-1f,0x1.f6c3f60000000p-1f,0x1.f6297c0000000p-1f,0x1.f58a2a0000000p-1f,0x1.f4e6020000000p-1f,0x1.f43d080000000p-1f,0x1.f38f3a0000000p-1f,0x1.f2dc9c0000000p-1f,0x1.f2252e0000000p-1f,0x1.f168f40000000p-1f,0x1.f0a7ee0000000p-1f,0x1.efe2200000000p-1f,0x1.ef178a0000000p-1f,0x1.ee482e0000000p-1f,0x1.ed740e0000000p-1f,0x1.ec9b2c0000000p-1f,0x1.ebbd8c0000000p-1f,0x1.eadb2e0000000p-1f,0x1.e9f4140000000p-1f,0x1.e908420000000p-1f,0x1.e817ba0000000p-1f,0x1.e7227c0000000p-1f,0x1.e6288e0000000p-1f,0x1.e529f00000000p-1f,0x1.e426a40000000p-1f,0x1.e31eae0000000p-1f,0x1.e212100000000p-1f,0x1.e100cc0000000p-1f,0x1.dfeae60000000p-1f,0x1.ded05e0000000p-1f,0x1.ddb13a0000000p-1f,0x1.dc8d7c0000000p-1f,0x1.db65260000000p-1f,0x1.da383a0000000p-1f,0x1.d906bc0000000p-1f,0x1.d7d0b00000000p-1f,0x1.d696160000000p-1f,0x1.d556f40000000p-1f,0x1.d4134c0000000p-1f,0x1.d2cb220000000p-1f,0x1.d17e760000000p-1f,0x1.d02d4e0000000p-1f,0x1.ced7ae0000000p-1f,0x1.cd7d980000000p-1f,0x1.cc1f0e0000000p-1f,0x1.cabc160000000p-1f,0x1.c954b20000000p-1f,0x1.c7e8e40000000p-1f,0x1.c678b20000000p-1f,0x1.c504200000000p-1f,0x1.c38b2e0000000p-1f,0x1.c20de20000000p-1f,0x1.c08c420000000p-1f,0x1.bf064e0000000p-1f,0x1.bd7c0a0000000p-1f,0x1.bbed7c0000000p-1f,0x1.ba5aa60000000p-1f,0x1.b8c38c0000000p-1f,0x1.b728340000000p-1f,0x1.b5889e0000000p-1f,0x1.b3e4d20000000p-1f,0x1.b23cd40000000p-1f,0x1.b090a40000000p-1f,0x1.aee04a0000000p-1f,0x1.ad2bc80000000p-1f,0x1.ab73240000000p-1f,0x1.a9b6620000000p-1f,0x1.a7f5840000000p-1f,0x1.a630900000000p-1f,0x1.a4678c0000000p-1f,0x1.a29a780000000p-1f,0x1.a0c95e0000000p-1f,0x1.9ef43e0000000p-1f,0x1.9d1b1e0000000p-1f,0x1.9b3e040000000p-1f,0x1.995cf20000000p-1f,0x1.9777ee0000000p-1f,0x1.958efe0000000p-1f,0x1.93a2240000000p-1f,0x1.91b1660000000p-1f,0x1.8fbcca0000000p-1f,0x1.8dc4520000000p-1f,0x1.8bc8060000000p-1f,0x1.89c7e80000000p-1f,0x1.87c4000000000p-1f,0x1.85bc500000000p-1f,0x1.83b0e00000000p-1f,0x1.81a1b20000000p-1f,0x1.7f8ecc0000000p-1f,0x1.7d78360000000p-1f,0x1.7b5df00000000p-1f,0x1.7940040000000p-1f,0x1.771e740000000p-1f,0x1.74f9480000000p-1f,0x1.72d0820000000p-1f,0x1.70a42a0000000p-1f,0x1.6e74440000000p-1f,0x1.6c40d60000000p-1f,0x1.6a09e60000000p-1f,0x1.67cf780000000p-1f,0x1.6591920000000p-1f,0x1.6350380000000p-1f,0x1.610b740000000p-1f,0x1.5ec3480000000p-1f,0x1.5c77ba0000000p-1f,0x1.5a28d20000000p-1f,0x1.57d6920000000p-1f,0x1.5581020000000p-1f,0x1.5328280000000p-1f,0x1.50cc080000000p-1f,0x1.4e6caa0000000p-1f,0x1.4c0a140000000p-1f,0x1.49a4480000000p-1f,0x1.473b500000000p-1f,0x1.44cf320000000p-1f,0x1.425ff00000000p-1f,0x1.3fed940000000p-1f,0x1.3d78220000000p-1f,0x1.3affa20000000p-1f,0x1.3884180000000p-1f,0x1.36058a0000000p-1f,0x1.3384000000000p-1f,0x1.30ff7e0000000p-1f,0x1.2e780c0000000p-1f,0x1.2bedb20000000p-1f,0x1.2960720000000p-1f,0x1.26d0540000000p-1f,0x1.243d5e0000000p-1f,0x1.21a7980000000p-1f,0x1.1f0f080000000p-1f,0x1.1c73b20000000p-1f,0x1.19d5a00000000p-1f,0x1.1734d60000000p-1f,0x1.14915a0000000p-1f,0x1.11eb340000000p-1f,0x1.0f426a0000000p-1f,0x1.0c97040000000p-1f,0x1.09e9060000000p-1f,0x1.0738780000000p-1f,0x1.0485620000000p-1f,0x1.01cfc80000000p-1f,0x1.fe2f640000000p-2f,0x1.f8ba4c0000000p-2f,0x1.f340580000000p-2f,0x1.edc1940000000p-2f,0x1.e83e0c0000000p-2f,0x1.e2b5d00000000p-2f,0x1.dd28f00000000p-2f,0x1.d797740000000p-2f,0x1.d2016c0000000p-2f,0x1.cc66e80000000p-2f,0x1.c6c7f40000000p-2f,0x1.c1249c0000000p-2f,0x1.bb7cf00000000p-2f,0x1.b5d1000000000p-2f,0x1.b020d40000000p-2f,0x1.aa6c800000000p-2f,0x1.a4b4100000000p-2f,0x1.9ef7900000000p-2f,0x1.9937140000000p-2f,0x1.9372a40000000p-2f,0x1.8daa500000000p-2f,0x1.87de280000000p-2f,0x1.820e380000000p-2f,0x1.7c3a900000000p-2f,0x1.7663400000000p-2f,0x1.7088500000000p-2f,0x1.6aa9d40000000p-2f,0x1.64c7dc0000000p-2f,0x1.5ee2700000000p-2f,0x1.58f9a40000000p-2f,0x1.530d840000000p-2f,0x1.4d1e200000000p-2f,0x1.472b880000000p-2f,0x1.4135c80000000p-2f,0x1.3b3cec0000000p-2f,0x1.35410c0000000p-2f,0x1.2f422c0000000p-2f,0x1.2940600000000p-2f,0x1.233bb80000000p-2f,0x1.1d34400000000p-2f,0x1.172a0c0000000p-2f,0x1.111d240000000p-2f,0x1.0b0d9c0000000p-2f,0x1.04fb800000000p-2f,0x1.fdcdc00000000p-3f,0x1.f19f900000000p-3f,0x1.e56ca00000000p-3f,0x1.d934f80000000p-3f,0x1.ccf8c80000000p-3f,0x1.c0b8200000000p-3f,0x1.b473280000000p-3f,0x1.a82a000000000p-3f,0x1.9bdcb80000000p-3f,0x1.8f8b800000000p-3f,0x1.8336680000000p-3f,0x1.76dd980000000p-3f,0x1.6a81300000000p-3f,0x1.5e21400000000p-3f,0x1.51bdf80000000p-3f,0x1.4557680000000p-3f,0x1.38edb80000000p-3f,0x1.2c81000000000p-3f,0x1.2011680000000p-3f,0x1.139f080000000p-3f,0x1.072a000000000p-3f,0x1.f564e00000000p-4f,0x1.dc70e00000000p-4f,0x1.c378500000000p-4f,0x1.aa7b700000000p-4f,0x1.917a600000000p-4f,0x1.7875800000000p-4f,0x1.5f6d000000000p-4f,0x1.4661100000000p-4f,0x1.2d52000000000p-4f,0x1.1440100000000p-4f,0x1.f656e00000000p-5f,0x1.c428c00000000p-5f,0x1.91f6600000000p-5f,0x1.5fc0000000000p-5f,0x1.2d86400000000p-5f,0x1.f693400000000p-6f,0x1.9215400000000p-6f,0x1.2d93400000000p-6f,0x1.921d000000000p-7f,0x1.921f000000000p-8f,0x0.0p+0f,-0x1.921f000000000p-8f,-0x1.921d000000000p-7f,-0x1.2d93800000000p-6f,-0x1.9215800000000p-6f,-0x1.f693800000000p-6f,-0x1.2d86600000000p-5f,-0x1.5fc0000000000p-5f,-0x1.91f6600000000p-5f,-0x1.c428e00000000p-5f,-0x1.f656e00000000p-5f,-0x1.1440100000000p-4f,-0x1.2d52100000000p-4f,-0x1.4661200000000p-4f,-0x1.5f6d000000000p-4f,-0x1.7875800000000p-4f,-0x1.917a700000000p-4f,-0x1.aa7b700000000p-4f,-0x1.c378600000000p-4f,-0x1.dc70f00000000p-4f,-0x1.f564e00000000p-4f,-0x1.072a080000000p-3f,-0x1.139f100000000p-3f,-0x1.2011700000000p-3f,-0x1.2c81080000000p-3f,-0x1.38edb80000000p-3f,-0x1.4557680000000p-3f,-0x1.51bdf80000000p-3f,-0x1.5e21480000000p-3f,-0x1.6a81300000000p-3f,-0x1.76dda00000000p-3f,-0x1.8336700000000p-3f,-0x1.8f8b800000000p-3f,-0x1.9bdcc00000000p-3f,-0x1.a82a000000000p-3f,-0x1.b473300000000p-3f,-0x1.c0b8280000000p-3f,-0x1.ccf8c80000000p-3f,-0x1.d935000000000p-3f,-0x1.e56ca00000000p-3f,-0x1.f19f980000000p-3f,-0x1.fdcdc00000000p-3f,-0x1.04fb800000000p-2f,-0x1.0b0d9c0000000p-2f,-0x1.111d280000000p-2f,-0x1.172a0c0000000p-2f,-0x1.1d34440000000p-2f,-0x1.233bbc0000000p-2f,-0x1.2940640000000p-2f,-0x1.2f422c0000000p-2f,-0x1.35410c0000000p-2f,-0x1.3b3cf00000000p-2f,-0x1.4135c80000000p-2f,-0x1.472b8c0000000p-2f,-0x1.4d1e240000000p-2f,-0x1.530d880000000p-2f,-0x1.58f9a80000000p-2f,-0x1.5ee2740000000p-2f,-0x1.64c7dc0000000p-2f,-0x1.6aa9d80000000p-2f,-0x1.7088540000000p-2f,-0x1.7663400000000p-2f,-0x1.7c3a940000000p-2f,-0x1.820e3c0000000p-2f,-0x1.87de2c0000000p-2f,-0x1.8daa540000000p-2f,-0x1.9372a80000000p-2f,-0x1.9937180000000p-2f,-0x1.9ef7940000000p-2f,-0x1.a4b4140000000p-2f,-0x1.aa6c840000000p-2f,-0x1.b020d80000000p-2f,-0x1.b5d1000000000p-2f,-0x1.bb7cf40000000p-2f,-0x1.c1249c0000000p-2f,-0x1.c6c7f40000000p-2f,-0x1.cc66e80000000p-2f,-0x1.d201700000000p-2f,-0x1.d797740000000p-2f,-0x1.dd28f00000000p-2f,-0x1.e2b5d40000000p-2f,-0x1.e83e100000000p-2f,-0x1.edc1940000000p-2f,-0x1.f340580000000p-2f,-0x1.f8ba4c0000000p-2f,-0x1.fe2f640000000p-2f,-0x1.01cfc80000000p-1f,-0x1.0485620000000p-1f,-0x1.07387a0000000p-1f,-0x1.09e9080000000p-1f,-0x1.0c97040000000p-1f,-0x1.0f426c0000000p-1f,-0x1.11eb360000000p-1f,-0x1.14915a0000000p-1f,-0x1.1734d60000000p-1f,-0x1.19d5a00000000p-1f,-0x1.1c73b40000000p-1f,-0x1.1f0f080000000p-1f,-0x1.21a79a0000000p-1f,-0x1.243d600000000p-1f,-0x1.26d0540000000p-1f,-0x1.2960720000000p-1f,-0x1.2bedb20000000p-1f,-0x1.2e780e0000000p-1f,-0x1.30ff800000000p-1f,-0x1.3384000000000p-1f,-0x1.36058a0000000p-1f,-0x1.3884180000000p-1f,-0x1.3affa20000000p-1f,-0x1.3d78240000000p-1f,-0x1.3fed960000000p-1f,-0x1.425ff20000000p-1f,-0x1.44cf320000000p-1f,-0x1.473b520000000p-1f,-0x1.49a44a0000000p-1f,-0x1.4c0a140000000p-1f,-0x1.4e6cac0000000p-1f,-0x1.50cc0a0000000p-1f,-0x1.53282a0000000p-1f,-0x1.5581040000000p-1f,-0x1.57d6940000000p-1f,-0x1.5a28d20000000p-1f,-0x1.5c77bc0000000p-1f,-0x1.5ec34a0000000p-1f,-0x1.610b760000000p-1f,-0x1.63503a0000000p-1f,-0x1.6591920000000p-1f,-0x1.67cf780000000p-1f,-0x1.6a09e60000000p-1f,-0x1.6c40d80000000p-1f,-0x1.6e74460000000p-1f,-0x1.70a42c0000000p-1f,-0x1.72d0840000000p-1f,-0x1.74f9480000000p-1f,-0x1.771e760000000p-1f,-0x1.7940060000000p-1f,-0x1.7b5df20000000p-1f,-0x1.7d78360000000p-1f,-0x1.7f8ece0000000p-1f,-0x1.81a1b40000000p-1f,-0x1.83b0e00000000p-1f,-0x1.85bc520000000p-1f,-0x1.87c4000000000p-1f,-0x1.89c7ea0000000p-1f,-0x1.8bc8060000000p-1f,-0x1.8dc4520000000p-1f,-0x1.8fbcca0000000p-1f,-0x1.91b1660000000p-1f,-0x1.93a2240000000p-1f,-0x1.958efe0000000p-1f,-0x1.9777f00000000p-1f,-0x1.995cf20000000p-1f,-0x1.9b3e040000000p-1f,-0x1.9d1b200000000p-1f,-0x1.9ef43e0000000p-1f,-0x1.a0c95e0000000p-1f,-0x1.a29a7a0000000p-1f,-0x1.a4678c0000000p-1f,-0x1.a630920000000p-1f,-0x1.a7f5840000000p-1f,-0x1.a9b6620000000p-1f,-0x1.ab73260000000p-1f,-0x1.ad2bca0000000p-1f,-0x1.aee04a0000000p-1f,-0x1.b090a60000000p-1f,-0x1.b23cd40000000p-1f,-0x1.b3e4d40000000p-1f,-0x1.b588a00000000p-1f,-0x1.b728340000000p-1f,-0x1.b8c38c0000000p-1f,-0x1.ba5aa60000000p-1f,-0x1.bbed7c0000000p-1f,-0x1.bd7c0a0000000p-1f,-0x1.bf064e0000000p-1f,-0x1.c08c420000000p-1f,-0x1.c20de40000000p-1f,-0x1.c38b2e0000000p-1f,-0x1.c504200000000p-1f,-0x1.c678b20000000p-1f,-0x1.c7e8e40000000p-1f,-0x1.c954b20000000p-1f,-0x1.cabc160000000p-1f,-0x1.cc1f0e0000000p-1f,-0x1.cd7d980000000p-1f,-0x1.ced7ae0000000p-1f,-0x1.d02d500000000p-1f,-0x1.d17e760000000p-1f,-0x1.d2cb220000000p-1f,-0x1.d4134c0000000p-1f,-0x1.d556f40000000p-1f,-0x1.d696160000000p-1f,-0x1.d7d0b00000000p-1f,-0x1.d906bc0000000p-1f,-0x1.da383a0000000p-1f,-0x1.db65260000000p-1f,-0x1.dc8d7c0000000p-1f,-0x1.ddb13c0000000p-1f,-0x1.ded0600000000p-1f,-0x1.dfeae60000000p-1f,-0x1.e100cc0000000p-1f,-0x1.e212100000000p-1f,-0x1.e31eae0000000p-1f,-0x1.e426a40000000p-1f,-0x1.e529f00000000p-1f,-0x1.e6288e0000000p-1f,-0x1.e7227e0000000p-1f,-0x1.e817ba0000000p-1f,-0x1.e908420000000p-1f,-0x1.e9f4140000000p-1f,-0x1.eadb2e0000000p-1f,-0x1.ebbd8c0000000p-1f,-0x1.ec9b2c0000000p-1f,-0x1.ed740e0000000p-1f,-0x1.ee482e0000000p-1f,-0x1.ef178a0000000p-1f,-0x1.efe2200000000p-1f,-0x1.f0a7f00000000p-1f,-0x1.f168f40000000p-1f,-0x1.f2252e0000000p-1f,-0x1.f2dc9c0000000p-1f,-0x1.f38f3a0000000p-1f,-0x1.f43d080000000p-1f,-0x1.f4e6040000000p-1f,-0x1.f58a2a0000000p-1f,-0x1.f6297c0000000p-1f,-0x1.f6c3f80000000p-1f,-0x1.f7599a0000000p-1f,-0x1.f7ea620000000p-1f,-0x1.f876500000000p-1f,-0x1.f8fd600000000p-1f,-0x1.f97f920000000p-1f,-0x1.f9fce40000000p-1f,-0x1.fa75580000000p-1f,-0x1.fae8e80000000p-1f,-0x1.fb57960000000p-1f,-0x1.fbc1600000000p-1f,-0x1.fc26460000000p-1f,-0x1.fc86460000000p-1f,-0x1.fce1600000000p-1f,-0x1.fd37900000000p-1f,-0x1.fd88da0000000p-1f,-0x1.fdd53a0000000p-1f,-0x1.fe1cb00000000p-1f,-0x1.fe5f3a0000000p-1f,-0x1.fe9cda0000000p-1f,-0x1.fed58e0000000p-1f,-0x1.ff09560000000p-1f,-0x1.ff38300000000p-1f,-0x1.ff621e0000000p-1f,-0x1.ff871c0000000p-1f,-0x1.ffa72e0000000p-1f,-0x1.ffc2520000000p-1f,-0x1.ffd8860000000p-1f,-0x1.ffe9ca0000000p-1f,-0x1.fff6200000000p-1f,-0x1.fffd880000000p-1f,-0x1.0000000000000p+0f,-0x1.fffd880000000p-1f,-0x1.fff6200000000p-1f,-0x1.ffe9ca0000000p-1f,-0x1.ffd8860000000p-1f,-0x1.ffc2520000000p-1f,-0x1.ffa72e0000000p-1f,-0x1.ff871c0000000p-1f,-0x1.ff621e0000000p-1f,-0x1.ff38300000000p-1f,-0x1.ff09560000000p-1f,-0x1.fed58e0000000p-1f,-0x1.fe9cda0000000p-1f,-0x1.fe5f3a0000000p-1f,-0x1.fe1cae0000000p-1f,-0x1.fdd53a0000000p-1f,-0x1.fd88da0000000p-1f,-0x1.fd37900000000p-1f,-0x1.fce15e0000000p-1f,-0x1.fc86460000000p-1f,-0x1.fc26460000000p-1f,-0x1.fbc1600000000p-1f,-0x1.fb57960000000p-1f,-0x1.fae8e80000000p-1f,-0x1.fa75580000000p-1f,-0x1.f9fce40000000p-1f,-0x1.f97f920000000p-1f,-0x1.f8fd600000000p-1f,-0x1.f8764e0000000p-1f,-0x1.f7ea620000000p-1f,-0x1.f7599a0000000p-1f,-0x1.f6c3f60000000p-1f,-0x1.f6297c0000000p-1f,-0x1.f58a2a0000000p-1f,-0x1.f4e6020000000p-1f,-0x1.f43d080000000p-1f,-0x1.f38f3a0000000p-1f,-0x1.f2dc9c0000000p-1f,-0x1.f2252e0000000p-1f,-0x1.f168f40000000p-1f,-0x1.f0a7ee0000000p-1f,-0x1.efe2200000000p-1f,-0x1.ef178a0000000p-1f,-0x1.ee482e0000000p-1f,-0x1.ed740e0000000p-1f,-0x1.ec9b2c0000000p-1f,-0x1.ebbd8c0000000p-1f,-0x1.eadb2e0000000p-1f,-0x1.e9f4140000000p-1f,-0x1.e908420000000p-1f,-0x1.e817ba0000000p-1f,-0x1.e7227c0000000p-1f,-0x1.e6288e0000000p-1f,-0x1.e529f00000000p-1f,-0x1.e426a40000000p-1f,-0x1.e31eae0000000p-1f,-0x1.e212100000000p-1f,-0x1.e100cc0000000p-1f,-0x1.dfeae60000000p-1f,-0x1.ded05e0000000p-1f,-0x1.ddb13a0000000p-1f,-0x1.dc8d7c0000000p-1f,-0x1.db65260000000p-1f,-0x1.da383a0000000p-1f,-0x1.d906bc0000000p-1f,-0x1.d7d0b00000000p-1f,-0x1.d696160000000p-1f,-0x1.d556f40000000p-1f,-0x1.d4134c0000000p-1f,-0x1.d2cb220000000p-1f,-0x1.d17e760000000p-1f,-0x1.d02d4e0000000p-1f,-0x1.ced7ae0000000p-1f,-0x1.cd7d980000000p-1f,-0x1.cc1f0e0000000p-1f,-0x1.cabc160000000p-1f,-0x1.c954b20000000p-1f,-0x1.c7e8e40000000p-1f,-0x1.c678b20000000p-1f,-0x1.c504200000000p-1f,-0x1.c38b2e0000000p-1f,-0x1.c20de20000000p-1f,-0x1.c08c420000000p-1f,-0x1.bf064e0000000p-1f,-0x1.bd7c0a0000000p-1f,-0x1.bbed7c0000000p-1f,-0x1.ba5aa60000000p-1f,-0x1.b8c38c0000000p-1f,-0x1.b728340000000p-1f,-0x1.b5889e0000000p-1f,-0x1.b3e4d20000000p-1f,-0x1.b23cd40000000p-1f,-0x1.b090a40000000p-1f,-0x1.aee04a0000000p-1f,-0x1.ad2bc80000000p-1f,-0x1.ab73240000000p-1f,-0x1.a9b6620000000p-1f,-0x1.a7f5840000000p-1f,-0x1.a630900000000p-1f,-0x1.a4678c0000000p-1f,-0x1.a29a780000000p-1f,-0x1.a0c95e0000000p-1f,-0x1.9ef43e0000000p-1f,-0x1.9d1b1e0000000p-1f,-0x1.9b3e040000000p-1f,-0x1.995cf20000000p-1f,-0x1.9777ee0000000p-1f,-0x1.958efe0000000p-1f,-0x1.93a2240000000p-1f,-0x1.91b1660000000p-1f,-0x1.8fbcca0000000p-1f,-0x1.8dc4520000000p-1f,-0x1.8bc8060000000p-1f,-0x1.89c7e80000000p-1f,-0x1.87c4000000000p-1f,-0x1.85bc500000000p-1f,-0x1.83b0e00000000p-1f,-0x1.81a1b20000000p-1f,-0x1.7f8ecc0000000p-1f,-0x1.7d78360000000p-1f,-0x1.7b5df00000000p-1f,-0x1.7940040000000p-1f,-0x1.771e740000000p-1f,-0x1.74f9480000000p-1f,-0x1.72d0820000000p-1f,-0x1.70a42a0000000p-1f,-0x1.6e74440000000p-1f,-0x1.6c40d60000000p-1f,-0x1.6a09e60000000p-1f,-0x1.67cf780000000p-1f,-0x1.6591920000000p-1f,-0x1.6350380000000p-1f,-0x1.610b740000000p-1f,-0x1.5ec3480000000p-1f,-0x1.5c77ba0000000p-1f,-0x1.5a28d20000000p-1f,-0x1.57d6920000000p-1f,-0x1.5581020000000p-1f,-0x1.5328280000000p-1f,-0x1.50cc080000000p-1f,-0x1.4e6caa0000000p-1f,-0x1.4c0a140000000p-1f,-0x1.49a4480000000p-1f,-0x1.473b500000000p-1f,-0x1.44cf320000000p-1f,-0x1.425ff00000000p-1f,-0x1.3fed940000000p-1f,-0x1.3d78220000000p-1f,-0x1.3affa20000000p-1f,-0x1.3884180000000p-1f,-0x1.36058a0000000p-1f,-0x1.3384000000000p-1f,-0x1.30ff7e0000000p-1f,-0x1.2e780c0000000p-1f,-0x1.2bedb20000000p-1f,-0x1.2960720000000p-1f,-0x1.26d0540000000p-1f,-0x1.243d5e0000000p-1f,-0x1.21a7980000000p-1f,-0x1.1f0f080000000p-1f,-0x1.1c73b20000000p-1f,-0x1.19d5a00000000p-1f,-0x1.1734d60000000p-1f,-0x1.14915a0000000p-1f,-0x1.11eb340000000p-1f,-0x1.0f426a0000000p-1f,-0x1.0c97040000000p-1f,-0x1.09e9060000000p-1f,-0x1.0738780000000p-1f,-0x1.0485620000000p-1f,-0x1.01cfc80000000p-1f,-0x1.fe2f640000000p-2f,-0x1.f8ba4c0000000p-2f,-0x1.f340580000000p-2f,-0x1.edc1940000000p-2f,-0x1.e83e0c0000000p-2f,-0x1.e2b5d00000000p-2f,-0x1.dd28f00000000p-2f,-0x1.d797740000000p-2f,-0x1.d2016c0000000p-2f,-0x1.cc66e80000000p-2f,-0x1.c6c7f40000000p-2f,-0x1.c1249c0000000p-2f,-0x1.bb7cf00000000p-2f,-0x1.b5d1000000000p-2f,-0x1.b020d40000000p-2f,-0x1.aa6c800000000p-2f,-0x1.a4b4100000000p-2f,-0x1.9ef7900000000p-2f,-0x1.9937140000000p-2f,-0x1.9372a40000000p-2f,-0x1.8daa500000000p-2f,-0x1.87de280000000p-2f,-0x1.820e380000000p-2f,-0x1.7c3a900000000p-2f,-0x1.7663400000000p-2f,-0x1.7088500000000p-2f,-0x1.6aa9d40000000p-2f,-0x1.64c7dc0000000p-2f,-0x1.5ee2700000000p-2f,-0x1.58f9a40000000p-2f,-0x1.530d840000000p-2f,-0x1.4d1e200000000p-2f,-0x1.472b880000000p-2f,-0x1.4135c80000000p-2f,-0x1.3b3cec0000000p-2f,-0x1.35410c0000000p-2f,-0x1.2f422c0000000p-2f,-0x1.2940600000000p-2f,-0x1.233bb80000000p-2f,-0x1.1d34400000000p-2f,-0x1.172a0c0000000p-2f,-0x1.111d240000000p-2f,-0x1.0b0d9c0000000p-2f,-0x1.04fb800000000p-2f,-0x1.fdcdc00000000p-3f,-0x1.f19f900000000p-3f,-0x1.e56ca00000000p-3f,-0x1.d934f80000000p-3f,-0x1.ccf8c80000000p-3f,-0x1.c0b8200000000p-3f,-0x1.b473280000000p-3f,-0x1.a82a000000000p-3f,-0x1.9bdcb80000000p-3f,-0x1.8f8b800000000p-3f,-0x1.8336680000000p-3f,-0x1.76dd980000000p-3f,-0x1.6a81300000000p-3f,-0x1.5e21400000000p-3f,-0x1.51bdf80000000p-3f,-0x1.4557680000000p-3f,-0x1.38edb80000000p-3f,-0x1.2c81000000000p-3f,-0x1.2011680000000p-3f,-0x1.139f080000000p-3f,-0x1.072a000000000p-3f,-0x1.f564e00000000p-4f,-0x1.dc70e00000000p-4f,-0x1.c378500000000p-4f,-0x1.aa7b700000000p-4f,-0x1.917a600000000p-4f,-0x1.7875800000000p-4f,-0x1.5f6d000000000p-4f,-0x1.4661100000000p-4f,-0x1.2d52000000000p-4f,-0x1.1440100000000p-4f,-0x1.f656e00000000p-5f,-0x1.c428c00000000p-5f,-0x1.91f6600000000p-5f,-0x1.5fc0000000000p-5f,-0x1.2d86400000000p-5f,-0x1.f693400000000p-6f,-0x1.9215400000000p-6f,-0x1.2d93400000000p-6f,-0x1.921d000000000p-7f,-0x1.921f000000000p-8f,0x0.0p+0f};
typedef struct {float level;uint8_t stage,gate;} cz_native_env_t;
typedef struct {float note[2];int32_t fine[2];uint32_t a4[2],inc[2],valid;} cz_native_freq_t;
typedef struct {cz_native_env_t native_eg[2][3];uint32_t vib_phase,vib_ticks,noise;float native_dc[2];cz_native_freq_t native_freq;} cz_voice_t;
typedef struct {float rate[8],level[8];uint8_t sustain,end;} cz_native_def_t;
typedef struct {cz_voice_t v[NPOLY];cz_native_def_t native_def[2][3];uint8_t native_raw[128],native_valid;} cz_part_t;
static cz_part_t *cz_part(uint32_t part);
static uint8_t cz_voice_was[NPART];
static uint8_t cz_patch[NPART][144];
static cz_voice_t *cz_voice(track_t*t,voice_t*v){return &cz_part((uint32_t)(t-trk)%NPART)->v[(uint32_t)(v-t->v)%NPOLY];}
static void cz_reset(track_t*t,voice_t*v){cz_voice_t*c=cz_voice(t,v);memset(c,0,sizeof *c);v->ph[0]=v->ph[1]=v->ph[2]=0;v->s[0]=v->s[1]=v->s[4]=0;if(!cz_voice_was[t-trk])v->s[2]=v->s[3]=0;c->noise=0x6D2B79F5u^(uint32_t)(v-t->v)*0x9E3779B9u;}
static void cz_native_block(track_t*t);
/* SPDX-License-Identifier: BSD-3-Clause
 * Modifications Copyright (C) 2026 Kerem Kilic (Ellic Studio)
 * Copyright (c) Devin Acker
 * Modifications Copyright (c) 2026 Kerem Kilic (Ellic Studio)
 * License: LICENSES/BSD-3-Clause-uPD933.txt
 * Native CZ-1 tone playback. Parameter encoding: Casio MIDI specification.
 * Chip envelope rate, phase functions and logarithmic DCA law follow the
 * independently documented uPD933 model by Devin Acker (MAME, BSD-3-Clause).
 * See docs/CZ1_SYSEX.md for sources and remaining calibration limits. */
#include "x0x/fastmath.h"

static const uint16_t CZ_AMP[128] = {
    0,4,5,5,5,6,6,7,7,8,8,9,9,10,11,12,
    12,13,14,15,16,18,19,20,22,23,25,27,29,31,33,36,
    38,41,44,47,51,54,58,63,67,72,77,83,89,96,103,110,
    118,127,136,146,157,168,180,194,208,223,239,257,275,296,317,340,
    365,392,421,451,484,520,558,598,642,689,739,793,851,914,980,1052,
    1129,1212,1300,1395,1497,1606,1724,1850,1985,2130,2286,2453,2632,2824,3031,3252,
    3490,3745,4019,4313,4628,4966,5329,5718,6136,6585,7066,7582,8136,8731,9369,10054,
    10789,11577,12423,13331,14305,15351,16473,17676,18968,20355,21842,23438,25151,26990,28962,31079
};
/* Tone-derived definitions live once per active part, outside voice state. */
/* Native control domains: DCO semitones, DCW 11-bit phase units, DCA 0..127.
 * Preserve the uPD933's rate and target laws, using fractional control time. */
static float cz_hw_rate(uint32_t raw,uint32_t e)
{
    uint32_t chip=(8u|(raw&7u))<<((raw&127u)>>3);
    float scale=e==0u ? 1.0f/(8.0f*131072.0f) : e==1u ? 1.0f/(4.0f*16384.0f) : 1.0f/(2.0f*131072.0f);
    return (float)chip*((float)CTL*40000.0f/FS)*scale;
}
static float cz_hw_target(uint32_t raw,uint32_t e)
{
    raw&=127u;
    if(e==0u)return (float)(raw&63u)*(raw&64u?2.0f:0.0625f);
    return (float)raw*(e==1u?8.0f:1.0f);
}
static __attribute__((noinline)) float cz_native_env_scaled(cz_native_env_t *e,const cz_native_def_t *d,uint32_t gate,float scale)
{
    float left=1.0f;
    if(e->gate && !gate && d->sustain<=d->end && e->stage<=d->sustain)e->stage=d->sustain+1u;
    e->gate=(uint8_t)gate;
    for(uint32_t k=0;k<8u && e->stage<=d->end;k++){
        uint32_t st=e->stage;float rate=d->rate[st]*scale,target=st==d->end?0.0f:d->level[st];
        float delta=target-e->level,dist=fm_fabsf(delta),step=rate*left;
        if(dist>step){e->level+=delta<0.0f?-step:step;break;}
        e->level=target;if(gate && st==d->sustain)break;e->stage++;
        if(dist>0.0f && rate>0.0f)left=fm_maxf(0.0f,left-dist/rate);
        if(left<=0.0f)break;
    }
    return e->level;
}
static void cz_native_note_on(track_t *t,voice_t *v)
{
    cz_native_block(t);
    cz_voice_t *c=cz_voice(t,v);
    float dc[2]={c->native_dc[0],c->native_dc[1]};
    cz_native_freq_t freq=c->native_freq;
    cz_reset(t,v); /* common phase/noise reset; zero has the same float representation */
    c->native_freq=freq; /* exact input keys also validate a retrigger */
    if(cz_voice_was[(uint32_t)(t-trk)%NPART]){c->native_dc[0]=dc[0];c->native_dc[1]=dc[1];}
    for(uint32_t l=0;l<2u;l++)for(uint32_t e=0;e<3u;e++)c->native_eg[l][e].gate=1;
}
static float cz_native_env_tick(cz_native_env_t *e,const cz_native_def_t *d,uint32_t gate)
{ return cz_native_env_scaled(e,d,gate,1.0f); }
static void cz_native_block(track_t *t)
{
    cz_part_t *c=cz_part((uint32_t)(t-trk)%NPART);
    if(!t->xf)memcpy(cz_patch[(uint32_t)(t-trk)%NPART],CZ_SELECTED[(uint32_t)t->p[P_E7]%4u],144);
    const uint8_t *b=cz_patch[(uint32_t)(t-trk)%NPART];
    if(c->native_valid && !memcmp(c->native_raw,b,128u))return;
    memcpy(c->native_raw,b,128u);c->native_valid=1;
    for(uint32_t l=0;l<2u;l++)for(uint32_t e=0;e<3u;e++){
        cz_native_def_t *d=&c->native_def[l][e];uint32_t base=CZ_ENV_BASE[l][e];
        d->end=b[CZ_ENV_END[l][e]]&7u;d->sustain=255;
        for(uint32_t k=0;k<8u;k++){
            uint32_t lev=b[base+2u*k+1u];
            d->rate[k]=cz_hw_rate(b[base+2u*k],e);d->level[k]=cz_hw_target(lev,e);
            if((lev&128u) && d->sustain==255u && k<d->end)d->sustain=(uint8_t)k;
        }
    }
}
static __attribute__((noinline)) float cz_native_amp(float level,uint32_t atten,uint32_t sense,uint32_t vel)
{
    float x=fm_clampf(level,0.0f,127.0f);uint32_t i=(uint32_t)x;
    float a=(float)CZ_AMP[i];
    if(i<127u)a+=(float)(CZ_AMP[i+1u]-CZ_AMP[i])*(x-(float)i);
    return a*(1.0f/32768.0f)*(float)(15u-atten)*(1.0f/15.0f)*
        (1.0f-(float)((127u-vel)*sense)*(1.0f/(127.0f*15.0f)));
}
/* Continuous evaluation of the chip's phase functions, with fp32 slopes.
 * uint32 phase retains tuning precision; no 11-bit truncation between samples. */
typedef struct { uint32_t wave[2],window; float dcw,pivot,k0,k1; } cz_hw_pd_t;
static __attribute__((noinline)) void cz_native_pd(cz_hw_pd_t *p,uint32_t word,float dcw)
{
    p->wave[0]=(word>>13)&7u;p->wave[1]=word&512u?(word>>10)&7u:p->wave[0];
    p->window=(word>>6)&7u;p->dcw=fm_clampf(dcw,0.0f,1023.0f);
    p->pivot=1024.0f-p->dcw;p->k0=1024.0f/p->pivot;p->k1=1024.0f/(2048.0f-p->pivot);
}
static __attribute__((noinline)) float cz_native_wave(const cz_hw_pd_t *b,uint32_t ph,uint32_t toggle)
{
    float pos=(float)ph*(2048.0f/4294967296.0f),half=pos>=1024.0f?pos-1024.0f:pos;
    float pivot=b->pivot,phase=0.0f,window=0.0f;
    switch(b->wave[toggle&1u]){
    case 0:phase=pos<pivot?pos*b->k0:1024.0f+(pos-pivot)*b->k1;break;
    case 1:phase=(half<pivot?half*b->k0:1024.0f)+(pos>=1024.0f?1024.0f:0.0f);break;
    case 2:phase=pos<pivot*2.0f?pos*b->k0:2048.0f;break;
    case 3:return 0.0f;
    case 4:phase=pos<pivot?pos*b->k0*2.0f:(pos-pivot)*b->k1*2.0f;break;
    case 5:phase=pos<1024.0f?pos:pos<pivot+1024.0f?1024.0f+half*b->k0:2048.0f;break;
    case 6:phase=pos*(1.0f+b->dcw*(1.0f/64.0f));break;
    default:phase=half<pivot?half*b->k0:2048.0f;break;
    }
    phase-=(float)(uint32_t)(phase*(1.0f/2048.0f))*2048.0f;
    switch(b->window){
    case 0:break;
    case 1:window=pos;break;
    case 2:window=pos<1024.0f?2048.0f-half*2.0f:half*2.0f;break;
    case 3:if(pos>=1024.0f)window=half*2.0f;break;
    case 4:window=pos<1024.0f?pos*2.0f:2048.0f;break;
    default:window=(1024.0f-half)*2.0f;break;
    }
    /* The normalized sine table is shared with FM6. Interpolate its cosine
     * quadrant in float: no chip-phase or PCM truncation before the mixer. */
    float x=phase*0.5f+256.0f;uint32_t i=(uint32_t)x;float f=x-(float)i;
    i&=1023u;
    float carrier=1.0f-(FM6_SIN[i]+(FM6_SIN[i+1u]-FM6_SIN[i])*f);
    return carrier*(1.0f-window*(1.0f/2048.0f))-1.0f;
}
static __attribute__((noinline)) float cz_native_vibrato(cz_voice_t *c, const uint8_t *b)
{
    uint32_t delay = (uint32_t)b[6] | (uint32_t)b[7] << 8, inc = (uint32_t)b[9] | (uint32_t)b[10] << 8;
    /* Approximate control clock; needs verification against a CZ-1. */
    c->vib_ticks++;
    if (c->vib_ticks < (uint32_t)((float)delay*FS/(200.0f*CTL))) return 0;
    c->vib_phase += (uint32_t)((float)inc*((float)CTL*200.0f*65536.0f/FS));
    int32_t x = (int32_t)(c->vib_phase >> 16), wave;
    if (b[4] & 32u) wave = 32767-x;
    else if (b[4] & 8u) wave = x < 32768 ? x*2-32768 : 98303-x*2;
    else if (b[4] & 4u) wave = x-32768;
    else wave = x < 32768 ? -32768 : 32767;
    uint32_t depth = (uint32_t)b[12] | (uint32_t)b[13] << 8;
    /* DEPTH 0 is encoded as machine depth 1 (Casio p. 85): keep it neutral. */
    return depth<=1u ? 0.0f : (float)wave*(float)depth*(1.0f/8388608.0f);
}
static __attribute__((noinline)) uint32_t cz_native_inc(float note,int32_t fine)
{
    float hz=(float)440u*fm_exp2f((note-69.0f)*(1.0f/12.0f));
    return (uint32_t)fm_clampf(hz*(4294967296.0f/FS)*(1.0f+(float)fine*(1.0f/4096.0f)),0.0f,2147483520.0f);
}
static __attribute__((noinline)) void cz_native_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m)
{
    if(!n)return;
    const uint8_t *b=cz_patch[(uint32_t)(t-trk)%NPART];
    cz_voice_t *c=cz_voice(t,v);const cz_native_def_t (*defs)[3]=cz_part((uint32_t)(t-trk)%NPART)->native_def;cz_hw_pd_t pd[2];
    uint32_t inc[2],ph[2]={v->ph[0],v->ph[1]},tg[2]={(uint32_t)v->s[0],(uint32_t)v->s[1]};
    float amp[2][2],step[2];
    uint32_t ls=b[0]&3u,first=ls==1u?1u:0u,last=ls>=2u?1u:first;
    float vib=cz_native_vibrato(c,b),oct=(b[0]>>2)==1u?12.0f:(b[0]>>2)==2u?-12.0f:0.0f;
    for(uint32_t l=0;l<2u;l++){
        uint32_t src=ls==2u?0u:l,off=src*57u;
        uint32_t av=15u-(b[20u+off]>>4),al=b[16u+off]>>4;
        int heard=l>=first && l<=last;
        if(heard)amp[l][0]=cz_native_amp(c->native_eg[l][2].level,al,av,v->vel);
        float pitch=cz_native_env_tick(&c->native_eg[l][0],&defs[src][0],v->gate);
        float depth=cz_native_env_tick(&c->native_eg[l][1],&defs[src][1],v->gate);
        uint32_t kfn=v->note>36u?v->note-36u:0u;
        float rate_scale=1.0f+(float)((b[16u+off]&15u)*kfn)*(1.0f/96.0f);
        cz_native_env_scaled(&c->native_eg[l][2],&defs[src][2],v->gate,rate_scale);
        /* Unheard envelopes keep advancing. Their stateless waveform,
         * amplitude and frequency preparation cannot affect next-note state. */
        if(!heard)continue;
        amp[l][1]=cz_native_amp(c->native_eg[l][2].level,al,av,v->vel);
        float difference=amp[l][1]-amp[l][0];
        step[l]=n==CTL?difference*(1.0f/CTL):difference/(float)n;
        uint32_t pv=15u-(b[54u+off]>>4),wv=15u-(b[37u+off]>>4);
        pitch*=1.0f-(float)((127u-v->vel)*pv)*(1.0f/(127.0f*15.0f));
        depth*=1.0f-(float)((127u-v->vel)*wv)*(1.0f/(127.0f*15.0f));
        uint32_t kf=b[18u+off],kfnote=v->note>36u?v->note-36u:0u;
        depth*=(!kf || !kfnote)?1.0f:96.0f/(float)(96u+kf*kfnote);
        float det=l?((float)b[3]+(float)(b[2]>>2)*(1.0f/64.0f))*(b[1]?-1.0f:1.0f):0.0f;
        float nt=fm_clampf((float)m->pitch16*(1.0f/16.0f)+oct+vib+pitch+det,0.0f,2047.0f/16.0f);
        cz_native_freq_t *f=&c->native_freq;
        if(!((f->valid>>l)&1u) || f->note[l]!=nt || f->fine[l]!=m->fine || f->a4[l]!=440u){
            f->note[l]=nt;f->fine[l]=m->fine;f->a4[l]=440u;f->inc[l]=cz_native_inc(nt,m->fine);f->valid|=1u<<l;
        }
        inc[l]=f->inc[l];
        uint32_t word=(uint32_t)b[14u+off]<<8|b[15u+off];
        float dep=depth+(float)(m->cutoff+m->shape-(64<<8))*(1.0f/32.0f);
        cz_native_pd(&pd[l],word,dep);
    }
    v->s[4]=(int32_t)(c->native_eg[first][2].level*256.0f);
    uint32_t modulation=(b[15]>>3)&7u;
    for(uint32_t i=0;i<n;i++){
        float line[2]={0.0f,0.0f};
        for(uint32_t l=first;l<=last;l++){
            uint32_t old=ph[l],delta=inc[l];
            if(l && modulation==3u){
                c->noise^=c->noise<<13;c->noise^=c->noise>>17;c->noise^=c->noise<<5;
                if(c->noise&1u)delta=(uint32_t)fm_minf((float)delta*(1625.0f/256.0f),2147483520.0f);
            }
            float raw=cz_native_wave(&pd[l],ph[l],tg[l]);ph[l]+=delta;
            if(ph[l]<old)tg[l]^=1u;
            c->native_dc[l]+=(raw-c->native_dc[l])*(1.0f/1024.0f);
            raw=(raw-c->native_dc[l])*0.5f;
            line[l]=raw*(amp[l][0]+step[l]*(float)i);
        }
        float sample=first==last?line[first]:modulation==4u?line[0]*line[1]*2.0f:(line[0]+line[1])*0.5f;
        float gain=((float)m->amp0+(float)(m->amp1-m->amp0)*(float)i*(1.0f/CTL))*(1.0f/32768.0f);
        out[i]+=(int32_t)(sample*gain*(4.0f*VOICE_FS));
    }
    v->ph[0]=ph[0];v->ph[1]=ph[1];v->s[0]=(int32_t)tg[0];v->s[1]=(int32_t)tg[1];
}

static int cz_native_done(track_t*t,voice_t*v){cz_voice_t*c=cz_voice(t,v);const uint8_t*b=cz_patch[(uint32_t)(t-trk)%NPART];uint32_t ls=b[0]&3u,a=ls==1?1:0,z=ls>=2?1:a;return c->native_eg[a][2].stage>(b[CZ_ENV_END[a][2]]&7u)&&c->native_eg[z][2].stage>(b[CZ_ENV_END[ls==2?0:z][2]]&7u);}
static const char*const CZ_NAMES[]={"BRASS 3","STRINGS 2","SYN STRINGS","FAT ENSEMBLE"};
static const preset_t CZ_PRESETS[]={
{"BRASS 3",{0,0,0,0,0,0,0,0},{0,70,127,60},0,0,FX(0,0,0,0)},
{"STRINGS 2",{0,0,0,0,0,0,0,1},{0,70,127,60},0,0,FX(0,0,0,0)},
{"SYN STRINGS",{0,0,0,0,0,0,0,2},{0,70,127,60},0,0,FX(0,0,0,0)},
{"FAT ENSEMBLE",{0,0,0,0,0,0,0,3},{0,70,127,60},0,0,FX(0,0,0,0)},
};
static const engine_t ENG_CZ={.name="CZ-1",.page_title={"CZ-1","TONE"},.edit={{"-",F_INT,0,0,0,0,0},{"-",F_INT,0,0,0,0,0},{"-",F_INT,0,0,0,0,0},{"-",F_INT,0,0,0,0,0},{"-",F_INT,0,0,0,0,0},{"-",F_INT,0,0,0,0,0},{"-",F_INT,0,0,0,0,0},{"TONE",F_ENUM,0,3,0,CZ_NAMES,0}},.presets=CZ_PRESETS,.npresets=4,.poly=4,.ownenv=1,.done=cz_native_done,.note_on=cz_native_note_on,.render=cz_native_render,.block=cz_native_block,.macro={P_E7,P_E7,P_E7,P_E7}};
