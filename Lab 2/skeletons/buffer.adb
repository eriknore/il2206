-- Package: Buffer

package body Buffer is
   protected body CircularBuffer is
      entry Push(Number : Integer)
	when not(In_Ptr = Out_Ptr - 1) is
      begin
	 In_Ptr := In_Ptr + 1;
         A(In_Ptr) := Number;
      end Push;

      entry Pop(Number : out Integer)
	when not(In_Ptr = Out_Ptr) is
      begin
         Out_Ptr := Out_Ptr + 1;
	 Number := A(Out_Ptr);
      end Pop;
   end CircularBuffer;
end Buffer;

