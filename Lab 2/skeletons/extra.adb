pragma Priority_Specific_Dispatching(FIFO_Within_Priorities, 2, 30);
pragma Priority_Specific_Dispatching(Round_Robin_Within_Priorities, 1, 1);


with Ada.Text_IO; use Ada.Text_IO;
with Ada.Real_Time; use Ada.Real_Time;

procedure Extra is

   Start : Time;
   Hyperperiod : Integer := 3600;
   TimeFIFO : Time_Span := Milliseconds(0);

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
      StopWatch : Time;
      Util : Float := 0.0;
   begin
      Next := Start;
      loop
         Next := Next + Milliseconds(Period);
	 if(Id = 99) then
		Put(" Util = ");
		--Duration_IO.Put(To_Duration(TimeFIFO), 3, 3);
		--Duration_IO.Put(To_Duration(Clock - Start), 3, 3);
	        Util := Float(To_Duration(TimeFIFO)) / Float(To_Duration((Clock - Start)));
		Put_Line(Float'Image(Util));
	 else
	 	StopWatch := Clock;
         	-- Some dummy function
         	Dummy := F(WorkLength);
         	Duration_IO.Put(To_Duration(Clock - Start), 3, 3);
         	Put(" : ");
         	Int_IO.Put(Id, 2);
         	Put_Line("");
	 	if(not(Prio = 1)) then
	    		TimeFIFO := TimeFIFO + (Clock - StopWatch);
	 	end if;
	 end if;
         delay until Next;
      end loop;
   end T;

   -- Example Task
   Task1 : T(1, 14, 300, 8); -- 9 => about 0.100 s of work
   Task2 : T(2, 12, 400, 8);
   Task3 : T(3, 10, 600, 8);
   Task4 : T(4, 1,  0,   8);
   Task5 : T(5, 1,  0,   8);
   Task6 : T(6, 1,  0,   8);
   Task99 : T(99, 30,  Hyperperiod,   0);
   
begin
   Start := Clock;
   null;
end Extra;
