__kernel void test_attribute_opencl_unroll_hint(int dummy) {
  __attribute__((opencl_unroll_hint)) for (int i = 0; i < 10; ++i);
}
