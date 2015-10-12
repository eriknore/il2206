-- THREE TASKS WATCHDOG
pragma Task_Dispatching_Policy(FIFO_Within_Priorities);

with Ada.Text_IO; use Ada.Text_IO;
with Ada.Real_Time; use Ada.Real_Time;

procedure WorkingOverloadDetection is

   Start : Time;

   WatchPeriod : Integer := 900; -- hyperperiod

   package Duration_IO is new Ada.Text_IO.Fixed_IO(Duration);
   package Int_IO is new Ada.Text_IO.Integer_IO(Integer);

   function F(N : Integer) return Integer;
   
   
   task type Watchdog is
      entry TaskFinished(Id : Integer);
      entry GetFinTasks(Nr : out Integer);
   end;

   task type Detect(HyperPeriod : Integer) is
      pragma Priority(30); -- highest
   end;

   task type T(Id: Integer; Prio : Integer; Period : Integer; WorkLength : Integer) is
      pragma Priority(Prio);
   end;

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

   task body Watchdog is
      Next : Time;
      T1 : Integer := 0;
      T2 : Integer := 0;
      T3 : Integer := 0;
      T4 : Integer := 0;
   begin
      Next := Start;
      Put_Line("Watchdog started");
      loop
       select
         accept TaskFinished(Id : Integer) do
	    if (Id = 1) then
		T1 := 1;
            elsif (Id = 2) then
		T2 := 1;
            elsif (ID = 3) then
		T3 := 1;
            elsif (ID = 4) then
	 	T4 := 1;
	    end if;
	 end TaskFinished;
       or
         accept GetFinTasks(Nr : out Integer) do
	    Nr := T1 + T2 + T3 + T4;
            T1 := 0;
      	    T2 := 0;
      	    T3 := 0;
      	    T4 := 0;
	 end GetFinTasks;
       end select;
      end loop;
   end Watchdog;

   Watch : Watchdog;

   task body Detect is
      Next : Time;
      FinTasks : Integer := 0;
   begin
      Next := Start;
      Put_Line("Detect started");
      Next := Next + Milliseconds(HyperPeriod);
      loop
	 delay until Next;
         Next := Next + Milliseconds(HyperPeriod);
         Watch.GetFinTasks(FinTasks);
         if(FinTasks < 4) then -- we have four tasks total
	      Put_Line("Overload detected!!");
         end if;
      end loop;
   end Detect;

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
	 Watch.TaskFinished(Id);
         delay until Next;
      end loop;
   end T;

   -- Example Task
   Task1 : T(1, 14, 300, 9); -- 9 => about 0.100 s of work
   Task2 : T(2, 12, 400, 9);
   Task3 : T(3, 10, 600, 9);
   Task4 : T(4, 8,  900, 17);

   D  : Detect(WatchPeriod);
   
begin
   Start := Clock;
   null;
end WorkingOverloadDetection;
