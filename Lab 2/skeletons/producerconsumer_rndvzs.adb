with Ada.Text_IO;
use Ada.Text_IO;

with Ada.Real_Time;
use Ada.Real_Time;

with Ada.Numerics.Discrete_Random;

with Buffer;
use Buffer;

procedure ProducerConsumer_Rndvzs is

   N : constant Integer := 40; -- Number of produced and comsumed variables

   -- Random Delays
   subtype Delay_Interval is Integer range 50..250;
   package Random_Delay is new Ada.Numerics.Discrete_Random (Delay_Interval);
   use Random_Delay;
   G : Generator;

   task Producer;

   task Consumer;

   task type Server is
	entry Push(Value : Integer);
	entry Pop(Value : out Integer);
   end Server;

   s : Server;

   task body Producer is
      Next : Time;
   begin
      Next := Clock;
      for I in 1..N loop
	 S.Push(I);
         -- Next 'Release' in 50..250ms
         Next := Next + Milliseconds(Random(G));
         delay until Next;
      end loop;
   end;

   task body Consumer is
      X : Integer;
      Next : Time;
   begin
      Next := Clock;
      for I in 1..N loop
	 S.Pop(X);
         Put_Line(Integer'Image(X));
         -- Next 'Release' in 50..250ms
         Next := Next + Milliseconds(Random(G));
         delay until Next;
      end loop;
   end;

  task body Server is
      Buf : CircularBuffer;
   begin
      loop
	 accept Push(Value : Integer) do
	     Buf.Push(Value);
	 end Push;
	 accept Pop(Value : out Integer) do
	     Buf.Pop(Value);
	 end Pop;
      end loop;
   end Server;

begin -- main task
   null;
end ProducerConsumer_Rndvzs;


