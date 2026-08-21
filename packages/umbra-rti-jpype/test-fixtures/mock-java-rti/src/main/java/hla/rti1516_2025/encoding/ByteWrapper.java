package hla.rti1516_2025.encoding;

/** Compact fixture implementation of the standard mutable byte cursor. */
public class ByteWrapper {
   private byte[] buffer;
   private int offset;
   private int position;
   private int limit;

   public ByteWrapper() { this(new byte[0]); }
   public ByteWrapper(int length) { this(new byte[length]); }
   public ByteWrapper(byte[] buffer) { this(buffer, 0, buffer.length); }
   public ByteWrapper(byte[] buffer, int offset) { this(buffer, offset, buffer.length - offset); }
   public ByteWrapper(byte[] buffer, int offset, int length) { reassign(buffer, offset, length); }

   public void reassign(byte[] value, int start, int length) {
      if (value == null || start < 0 || length < 0 || start + length > value.length) {
         throw new ArrayIndexOutOfBoundsException("Invalid ByteWrapper bounds");
      }
      buffer = value;
      offset = start;
      position = start;
      limit = start + length;
   }
   public void reset() { position = offset; }
   public void verify(int length) {
      if (length < 0 || position + length > limit) throw new ArrayIndexOutOfBoundsException(position + length);
   }
   public final int getInt() {
      verify(4);
      int value = ((buffer[position] & 0xff) << 24) | ((buffer[position + 1] & 0xff) << 16)
         | ((buffer[position + 2] & 0xff) << 8) | (buffer[position + 3] & 0xff);
      position += 4;
      return value;
   }
   public final int get() { verify(1); return buffer[position++] & 0xff; }
   public final void get(byte[] destination) { verify(destination.length); System.arraycopy(buffer, position, destination, 0, destination.length); position += destination.length; }
   public void putInt(int value) { verify(4); put(value >>> 24); put(value >>> 16); put(value >>> 8); put(value); }
   public void put(int value) { verify(1); buffer[position++] = (byte) value; }
   public void put(byte[] source) { put(source, 0, source.length); }
   public void put(byte[] source, int sourceOffset, int count) { verify(count); System.arraycopy(source, sourceOffset, buffer, position, count); position += count; }
   public final byte[] array() { return buffer; }
   public final int getPos() { return position; }
   public int remaining() { return limit - position; }
   public final void advance(int count) { verify(count); position += count; }
   public void align(int alignment) { while ((position - offset) % alignment != 0) advance(1); }
   public ByteWrapper slice() { return new ByteWrapper(buffer, position, remaining()); }
   public ByteWrapper slice(int length) { verify(length); return new ByteWrapper(buffer, position, length); }
}
