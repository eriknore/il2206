pragma Task_Dispatching_Policy(FIFO_Within_Priorities);

with Ada.Text_IO; use Ada.Text_IO;
with Ada.Real_Time; use Ada.Real_Time;

procedure RMS is

   Start : Time;

   package Duration_IO is new Ada.Text_IO.Fixed_IO(Duration);
   package Int_IO is new Ada.Text_IO.Integer_IO(Integer);

   function F(N : Integer) return Integer;

   function F(N : Integer) return Integer is
      X : Integer := 0;
   begin
      for Index in 1..N loop
         for I in 1..5000000 loop
            X := I;
         end loop;
      end loop;
      return X;
   end F;   

   task type T(Id: Integer; Prio : Integer; Period : Integer; WorkLength : Integer) is
      pragma Priority(Prio);
   end;

   task body T is
      Next : Time;
      Dummy : Integer;
   begin
      Next := Start;
      loop
         Next := Next + Milliseconds(Period);
         -- Some dummy function
         Dummy := F(WorkLength);
         Duration_IO.Put(To_Duration(Clock - Start), 3, 3);
         Put(" : ");
         Int_IO.Put(Id, 2);
         Put_Line("");
         delay until Next;
      end loop;
   end T;

   -- Example Task
   Task1 : T(1, 14, 300, 9); -- 9 => about 0.100 s of work
   Task2 : T(2, 12, 400, 9);
   Task3 : T(3, 10, 600, 9);
   --Task4 : T(4, 8,  900, 17);
   
begin
   Start := Clock;
   null;
end RMS;
