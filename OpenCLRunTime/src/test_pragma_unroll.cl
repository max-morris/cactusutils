__kernel void test_pragma_unroll(int dummy) {
  _Pragma("unroll") for (int i = 0; i < 10; ++i);
}
