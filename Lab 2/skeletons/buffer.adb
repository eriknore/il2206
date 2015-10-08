-- Package: Buffer

package body Buffer is
   protected body CircularBuffer is
      entry Push(Number : Integer)
	when not(In_Ptr = Out_Ptr - 1) is
      begin
	 In_Ptr := In_Ptr + 1;
         A(In_Ptr) := Number;
      end Push;

      entry Pop
	when not(In_Ptr = Out_Ptr) is
      begin
         Out_Ptr := Out_Ptr + 1;
      end Pop;

      function Read return Integer is
      begin
         return A(Out_Ptr); 
      end Read;
   end CircularBuffer;
end Buffer;

