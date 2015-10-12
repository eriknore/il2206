pragma Task_Dispatching_Policy(FIFO_Within_Priorities);
--pragma Profile(Ravenscar);

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

   task type T(Id: Integer; Period : Integer; WorkLength : Integer) is
      pragma Priority(Id);
   end;

   task body T is
      Next : Time;
      Dummy : Integer;
   begin
      Next := Start;
      --Next := Next + Milliseconds(1000);
      --delay until Next;
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
   Task1 : T(10, 600, 10); -- 8 => about 0.100 s of work
   Task2 : T(12, 400, 10);
   Task3 : T(14, 300, 10);
begin
   Start := Clock;
   null;
end RMS;
