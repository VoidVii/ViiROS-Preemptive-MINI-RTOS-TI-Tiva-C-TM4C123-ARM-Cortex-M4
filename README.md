# ViiROS – Präemptives MINI RTOS für ARM Cortex-M4 (TM4C123GL - TM4C123GH6PM)

<table>
  <tr>
    <td width="60%">
      <b>Übersicht:</b><br>
      ViiROS ist ein eigenständig von Grund auf entwickelter minimaler, präemptiver Echtzeitkernel für ARM Cortex-M4. 
	  Entwickelt als Lernprojekt zur Vertiefung von RTOS-Konzepten und Hardware-naher Programmierung.
	  Der Fokus liegt auf prioritätsbasiertem Scheduling, Context Switch (PendSV) und SysTick-Interrupt als System-Takt – vollständig ohne externe Bibliotheken
    </td>
    <td width="40%">
      <img width="450" height="370" alt="grafik" src="https://github.com/user-attachments/assets/e00fc2a9-9b90-4142-ba76-9efb73b29d35" />
    </td>
  </tr>
</table>


## Proof of Concept
<img width="1157" height="230" alt="AnalyzerViewOnPreemptiveScheduling2" src="https://github.com/user-attachments/assets/a66bb215-a624-423a-8709-959575a12aff" />

Die PulsView-Messung veranschaulicht die prioritätsbasierte, präemptive Arbeitsweise des Kernels.

Die Prioritäten nehmen von oben nach unten ab:
- Red = 3, Blue = 2, Green = 1, Idle = 0

Pin-Belegung (PORT F):
- SysTick = Pin 4 | Red = Pin 1 | Blue = Pin 2 | Green = Pin 3 | Idle = Pin 0

Zu erkennen ist:

- Der SysTick-Interrupt tritt im festen Takt von 1 ms auf
- Der Scheduler startet zuverlässig immer den Thread mit der höchsten Priorität
- Threads werden gemäß den Systemregeln durch höher priorisierte Threads präemptiv unterbrochen
- Der Context Switch nach dem Scheduling erfolgt korrekt
- **Unterbrochene Threads setzen ihre Ausführung exakt an der Stelle der Unterbrechung fort**
- Der Idle-Thread läuft nur, wenn kein anderer Thread aktiv ist

Zur Visualisierung wurde der SysTick-Interrupt-Handler so erweitert, dass GPIOF Pin 4 getriggert wird.
Der Idle-Thread toggelt entsprechend Pin 0.

Die Thread-Handler selbst sind so implementiert, dass sie die jeweiligen LEDs mehrfach toggeln und dadurch die Ausführung sichtbar machen.

Weitere Details zum Setup befinden sich am Ende der Readme.

## Ziel des Projekts
Ziel war es, die grundlegenden Mechanismen eines präemptiven Schedulers praktisch zu verstehen und selbst umzusetzen.

Der Fokus lag dabei insbesondere auf:
- Prioritätbasiertem Scheduling
- Conext Switch mit PendSV in Assembler
- Nutzung von MSP und PSP
- Verständnis Arm Cortex-M4 Architektur

## Erkenntnisse 
- Context Switch ist deutlich komplexer als es auf den ersten Blick wirkt
- Fehler im Stack-Handling führen schnell zu schwer nachvollziehbaren Problemen
- Das Zusammenspiel von Interrupts, Scheduler und CPU-Modi ist entscheidend für ein stabiles System

## Eingenschaften des Systems:
- prioritätenbasierte parallele Ausführung mehrere Threads
- Context Switch über PendSV in Assemlber
- MSP für Interrupts (Handler Mode)
- PSP für Threads (Thread Mode)
- SysTick als 1ms Zeitbasis

## Features
- Thread-System
	- Thread Control Blocks (TCB)
	- Stack pro Thread
	- fabrizierter Initial-Interrupt-Stack-Frame auf PSP (als wäre Thread vom Interrupt unterbrochen)
   		- XPSR, PC, LR, R12, R3-R0 (Caller Saved Register)
     	- R11 - R4 (Calle Saved Register)
    - Magic Numbers zur Stack und Overflow Erkennung  
	- Parallele Ausführung (Multitasking-System)

- Kernel / Scheduler:
	- Präemptiver Scheduler
	- Prioritätenbasiert 
	- Bitmasken (32 Bit -> 32 Prioritäten) mit (32 - CLZ) (count leading zeros) -> O(1):
		- readyMask     0b01000000000000000000000100000000 (Prio 31, 9 ready)
		- blockedMask  	0b00000000000000000000000000100001 (Prio 6, 1 blocked)

- Timing & Blocking
	- SysTick 1ms Zeitbasis
 		- ViiROS_BlockWatch(): Block Threads -> blockedMask
   		- ViiROS_Scheduler(): Blocktime Management, Unblock Threads -> ebenfalls über CLZ

- CPU-Modus-Handling:
	- Handler Mode u. Thread Mode
	- MSP (Main Stack Pointer) für Interrupts
	- PSP (Process Stack pointer) für Threads
	- Spacial Register CONTROL (= 0x02 -> SPSEL; Umschalten von MSP -> PSP)
	- EXC_RETURN LR = 0xFFFFFFFD (Interrupt kehrt in den Thread-Modus zurück und nutzt PSP)


- Kontextwechsel (Context Switch):
	- PendSV als Context Switch Interrupt in assembly
	- Manuelles Sichern/Laden der Calle Save Register (R4-R11) auf/vom PSP
 		- STMDB R1!, {R4-R11} -> R4-R11 sichern
   		- LDMIA R1, {R4-R11} -> R4-R11 laden
	- Manuelles PSP setzen
 		- MRS       R1, PSP  -> R1 = PSP (sichern)
   		- MSR       PSP, R0  -> PSP = R0 (laden)
 		- nutzen mit (CONTROL // EXC_RETURN)



## Quick Start
ViiROS kann mit bis zu 32 Threads verschiedener Prioritäten arbeiten. 

Für die Threads muss:
- Thread (TCB)
- Stack mit Stackgröße
- Thread-Handler
 
in main.c deklariert und programmiert werden.

**Beispiele sind in main.c zu sehen!**

Der **Idle-Thread** wird bei Systemstart automatisch gestartet. 
Das Starten der Anwender-Threads erfolgt mit:

    ViiROS_Thread Red; 					
    static uint32_t stack_Red[80];		 
	
	/* Thread_Handler programmieren*/
    void Red_Handler(void)
    {
      while(1){
       /* Code */
    }
  	/* Thread starten */ 
    ViiROS_ThreadStart(&Red,
                    	Red_Handler,
                        Priority,
                        stack_Red, sizeof(stack_Red));
						 
Die **Stackgröße** sollte zur Sicherheit **nicht zu gering** gewählt werden, da schon der Thread mindest 62 Bytes alleine für den Interrupt-Stack-Frame benötigt.
Der **Thread-Handler** muss eine *while(1)-Loop* haben und darf nicht aus dieser raus. Das System unterschtützt noch **keine dynamische Thread-Löschung**!!!

Nach vollständiger Konfiguration und Initialisierung der Komponenten wird die Kontrolle über das System an ViiROS übergeben:

    ViiROS_Run()
  
**Wichtig:** ViiROS_Run() kehrt nie zurück – das System läuft von nun an im Thread-Modus auf PSP.


## Projektstruktur

Datei:			
ViiROS.c/h 	->	Kernel, Scheduler, Blocking
SysTick.c/h	->	Zeitbasis (1ms)
GPIO.c/h	->	LED, Taster 
main.c		->	Beispiel-Threads


## Hardware & Toolchain
- IAR Embedded Workbench (Arm)
- Tiva C Series LaunchPad TM4C123GXL
- TI TM4C123GH6PM (ARM Cortex-M4)
- On-board LEDs
- User switch

## Wie es funktioniert
Das System führt nach dem Starten der einzelnen Threads mit **ViiROS_Run()** gleich zu Beginn nach dem **PendSV** den Thread mit der **höchsten Priorität** in der **readyMask** aus.

- ViiROS_Run() -> Scheduler() -> current-thread == NULL, next-thread == highest ready prio -> PendSV -> set CONTROL, LR, load next-thread -> start next-thread on PSP 
	
Zuvor wird durch den Aufruf des Schedulers der **nächste Thread to run** ausgewählt und der PendSV getriggert. Bei dem ersten Context Switch im PendSv wird das System von MSP auf PSP umgestellt. Sicher gestellt wird dies durch die Tatsache das noch kein Thread ausgeführt wurde und der Current-Thread ein NULL-Pointer ist. Bei dem aller ersten PendSV wird einmalig das Special Register CONTROL = 0x02 und der LR = 0xFFFFFFFD gesetzt. Beides ist erforderlich damit der Wechsel von MSP auf PSP gelingt und die Threads auf dem PSP laufen.

### Thread-Zustände & Blocking
Die Threads haben zwei Zustände **Ready** und **Blocked**. Beide werden in jeweils einer 32-Bit Bitmaske repräsentiert. 
Der **Index** der Bits steht für die **Priorität** der Threads:

*                  Bit:         31 30 29 ... 5 4 3 2 1 0 <-- (priority - 1)
*                  Value:        0  0  0 ... 0 1 0 1 0 1 

- "Ready" - Threads -> readyMask
- "Blocked" - Threads -> blockedMask

Zum Scannen der Masken wird die CLZ() [Count leading zeros] auf ARM verwendet und ermöglicht das Auslesen in einem Durchlauf.

Geblockt werden die Threads mit **ViiROS_BlockTime()**. Hier wird das blocktime-Attribut des TCBs auf die Zeit die der Thread geblockt sein muss gesetzt. Im Anschluss wird der Thread in readyMask auf 0 und in blockedMask auf 1 gesetzt. Mit dem anschließenden Aufruf des Schedulers wird sofort der nächste ready Thread gestartet.

### Architekturdiagramm & Periodischer Ablauf

	┌────────────────────────────────────────────┐	
	│              ViiROS KERNEL                 │		Periodischer Ablauf:
	├────────────────────────────────────────────┤	
	│                                            │		SysTick (1 ms) [höchste Interrupt-Prio]		
	│   SYSTICK ──► SCHEDULER ──► PENDSV         │ 			|
	│   (1ms)        (O(1))       (Context Sw)   │		BlockWatch (dekrementiert blocktime, set/clear readyMask/blockedMask)
	│                                 │          │			|	
	│                                 ▼          │		Scheduler (LOG2(readyMask) → höchste Prio)
	│   ┌─────────────────────────────────┐      │			|
	│   │      THREAD MANAGEMENT          │      │		PendSV (Context Switch) [niedrigste Interrupt-Prio]		
	│   │  ┌───────────┬───────────┐      │      │			|
	│   │  │  READY    │  BLOCKED  │      │      │		Thread läuft auf PSP
	│   │  │ (Bitmask) │(Bitmask)  │      │      │
	│   │  └───────────┴───────────┘      │      │
	│   └─────────────────────────────────┘      │
	│                    │                       │
	│                    ▼                       │
	│   ┌─────────────────────────────────┐      │
	│   │      HARDWARE (TM4C123GH6PM)    │      │
	│   │   LED   │   Switch   │ SysTick  │      │
	│   └─────────────────────────────────┘      │
	└────────────────────────────────────────────┘

Die Grundstruktur von ViiROS wiederholt sich jede 1 ms mit dem SysTick der den Puls des Systems vorgibt:

1. Systick jede 1 ms
	- Aufrufen von BlockWatch() u. Scheduler()
2. BlockWatch() =>  **Thread->blocktime** **runtergezählt**
	- Wird **blocktime == 0** => Thread in blockedMask löschen und in readyMask setzen
3. Scheduler() => Update nächsten ready Thread mit höchster Priorität
	- Trigger Context Switch durch setzen von PendSV-Pending-Bit wenn momentatner Thread nicht dem nächsten Thread entspricht
4. Nächsten Thread ausführen
	
### Scheduler

	/**
	*@brief Preemptive Scheduler (Priority, Blocking)
	*@note LOG2(readyMask) >> get highest priority in one operation
	*@note Triggers PendSV
	*/
	void ViiROS_Scheduler(void)
	{
	  ViiROS_Thread *current = ViiROS_current; 
	  ViiROS_Thread *next;
	  
	  if(ViiROS_readyMask == 0U) 
	  {
	    next = Active_Thread[0]; 
	  }
	  else 
	  {
	    uint32_t highPrio = LOG2(ViiROS_readyMask);
	    next = Active_Thread[highPrio]; 
	  }
	  
	  if(current != next) 
	  {
	    ViiROS_next = next; 
	    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
	  } 
	}

#### Die Funktionsweise des Schedulers besteht aus zwei simplen Abfragen:
1. **readyMask == 0 (Idle-Bedingung)**
	- so soll der Idle-Thread als nächstes laufen
2. **readyMask != 0**
	- finde mit LOG2 = 32 - CLZ(readyMask) - nächst höchste ready Priorität
	- Z. B.: readyMask = 0b0000 1001 -> 32 - 28 = 4 -> höchste ready Prio = 4	 
3. Wenn Nächster-Thread ungleich Momentanter-Thread
	- Setze das Pendingbit von PendSV um den Context Switch triggern


### Context Switch (PendSV)
#### Beim ersten PendSV in System:
- Current-Thread = NULL -> springt zu PendSV_first_run:
  	- Setze Special Register CONTROL = 0x02 (SPSEL (Bit 1) = 1 ==> sagt der CPU "Benutze PSP" (Process Stack Pointer)
  	- Setze LR = 0xFFFFFFFD (EXC_RETURN) ==> Spring aus dem Interrupt und nutze PSP
  	- ISB ==> notwendig um Änderungen zu übernehmen (Instruction Synchronization Barrer)

**Wichtig:** **CONTROL = 0x02** und **LR = 0xFFFFFFFD** in Verbindung sind wichtig. Da der erste PendSV-Interrupt-Aufruf aus main() erfolgt steht zu nächst der falsche Wert im LR-Register!!! Wird der Wert in LR nicht auf 0xFFFFFFFD gesetzt so kehrt der PendSV-Interrupt wieder zurück zu main.c weil die CPU weiterhin den MSP nutz und beendet das Programm. 

#### Normaler PendSV-Durchlauf
Nach erstem PendSV wird der Branch **PendSV_first_run nie wieder aufgerufen!** Jetzt wird immer der **normale Context Switch** durchlaufen.
- Speichere den Kontext des aktuellen Threads
	- MRS       R1, PSP (move special register into general purpose register) => lade R1 = PSP
 	- STMDB     R1!, {R4-R11} (store multible, decrement before) => dekrementiere R1 und speichere R11 auf PSP-Stack -> --R1 save R10
  	- Reihenfolge ist hier R11 -> R4
	- STR       R1, [R0]  => [R0] = R1 => R0([Thread_TCB] + Offset 0x00 --> Thread->SP (erstes Atrribut im TCB)) = R1
		- Speichere PSP im Thread->SP

- Lade den nächsten Thread
	- Lad in R0 den Thread->SP (Darin ist PSP vom nächsten Thread gespeichert)
 	- LDMIA     R0!, {R4-R11} => Lade R4 vom Stack in R4 und inkrementiere R0 (Reihenfolge beim Laden ist R4 - R11)
  	- R0 zeigt nach dem Laden auf **R0 auf dem PSP-Stack** und ist bereit zum Laden der Hardware Register
  	- MSR       PSP, R0 => Setze PSP auf neuen R0
  	- STR       R1, [R2] => setze ViiROS_current auf ViiROS_next

#### Interrupt-Stack-Layout
  
						LOW ADDRESS
						--------------
				--------R4 <-- PSP - nach STMDB     R1!, {R4-R11} //  vor LDMIA     R0!, {R4-R11} 
						R5
						R6
		   Software stk R7			Werden im PendSV gesichert und geladen
						R8
						R9
						R10
			   ---------R11
	PSP nach BX LR  --> R0 <-- PSP - vor STMDB     R1!, {R4-R11} // nach LDMIA     R0!, {R4-R11} 
						R1
						R2
		   Hardware stk R3
						R12			Werden von Hardware automatisch beim Eintritt des Interrupts auf PSP gesichert 
						LR			und beim Austritt nach BX LR geladen
						PC
			   --------xPSR <-- PSP nach dem laden der Hardware-Register
						--------------
						HIGH ADDRESS

## Herausforderungen & Lösungen
### Stack-Korruption durch falsche Initialisierung von Current-Thread (ViiROS_current):
  - Problem: ViiROS_current = Idle-Thread vor erstem Context Switch
  - Folge:   SP im Idle-TCB wurde mit MSP (Main Stack Pointer) im PendSV_Handler überschrieben.
    		Die Abfrage im PendSv für den Sonderfall erster Druchlauf wurde nicht erfüllt.
    		CONTROLL und LR wurden nicht richtig gesetzt.
  - Lösung:  ViiROS_current = NULL, somit wurde der Wechsel von MSP zu PSP sichergestellt und der Idle-Thread nicht korumpiert

### Stack Overflow durch zu kleine Thread-Stacks:
  - Problem:  Stackgröße für die einzelnen Threads zu klein gewählt.
  - Folge:    Der Stack wuchs in den Bereich des Arrays der aktiven Threads (Active_Thread[]).   
  			  Array-Einträge wurden überschrieben und gestartete Threads zerstört.
    		  Und löste weitere Probleme wie korumpierte PC werte und BusFault aus.
  - Lösung:   Zu kleine Größe wurde mit Hilfe von Magic Numbers (0xCAFEBABE) erkannt und der Stackgröße schrittweise erhöht.

### Hard Faults durch nicht initialisierte GPIO-Pins
  - Problem:  Ein Thread hat auf nicht initialisierte GPIO Pins zugegriffen.
  - Folge:    Hard Fault, sobald der Thread auf die LED zugriff.
  - Lösung:   In main() die GPIO Konfiguration ergänzen.

### Inkorrekter EXC_RETURN in LR im PendSv
  - Problem:  Falscher EXC_RETURN in LR während des PendSV durch den Wechsel von main() -> PendSV
  - Folge:    CPU kehrte nach PendSV zurück zu main() und beendete das Programm.
  - Lösung:   Zusätzlich zu CONTROL den korrekten EXC_RETURN in LR schreiben - LR = 0XFFFFFFFD => "return" in Thread-Mode und nutze PSP!!!


## Lernhintergrund
Dieses Projekt baut konzeptionell auf den Inhalten des **Modern Embedded Systems Programming Course** von **Miro Samec (Quantum Leaps)** auf, den ich intensiv studiert habe.  Die dort vermittelten Prinzipien – insbesondere zu präemptivem Scheduling, PendSV, Stack-Management und RTOS-Interna – habe ich verstanden und in eine **eigene, eigenständige Implementierung** überführt.

Der Code ist keine 1:1-Umsetzung von Beispielen, sondern meine eigene Arbeit, in der ich die Konzepte angewendet, weiterentwickelt und an meine Anforderungen angepasst habe. Die Lehren aus diesen Bemühungen und die eigenständige Umsetzung ermöglichten es mir, die Themen nicht nur anzuwenden, sondern auch auf einer tieferen Ebene zu verstehen und zu verinnerlichen.

Das Projekt Preemptive scheduler ViiROS baut auf meinem vorherigen Projekt Cooperative scheduler auf und repräsentiert meine erstes größere Projekt, **von 0 zum Minit-RTOS**, im Thema Embedded Systems. 

## Debugging - Screenshots - Bugs

### Zu kleine Stackgröße --> Stack Overflow -> BusFault, Active-Thread[] korumpiert

<img width="650" height="350" alt="ZuKleineStackProbleme" src="https://github.com/user-attachments/assets/0b4f6e69-1684-4dea-a1d9-7c51c62d8116" />

### ViiROS_current =! NULL beim System-Start
1. Falscher Current-Thread zum System-Start
2. ViiROS_IDLE (Current-Thread) initialisierter Stack mit ViiROS_Idle->SP 0x2000´0248 zeigt auf R4 = 0xCAFEBABE
3. ViiROS_Idle->SP wurde im PendSV beim speichern des PSP in Idle->Sp mit flaschen Wert überschrieben => Idle-Thread zerstört
5. CBZ (Abfrage cuurent == 0?) keinen Sprung zum PendSV_first_run
	- CONTROL(0x02) und LR(0xFFFF FFFD) wurden nicht gesetzt!
	- Kein Wechsel von MSP auf PSP
 	- Nach Interrupt geht zurück in ViiROS_Run() in main.c

**INFO:** Wenn LR nicht manuell im PendSV auf 0xFFFF FFFD gesetzt wird kehrt die CPU ebenfalls in main() zurück!!

<img width="850" height="578" alt="CurrentFalschGesetzt" src="https://github.com/user-attachments/assets/fbf25b5e-af18-447d-ad58-934eb94dcfe3" />

# Setup
1. Laptop -> IAR Workbench (Arm), Pulsview
2. Dev-Board Tiva C Series LaunchPad TM4C123GXL -> LEDs, User Switch und mehr
3. Logic Analyzer 24MHz 8 Channel


<img width="850" height="550" alt="grafik" src="https://github.com/user-attachments/assets/313899da-e5b0-461a-91c9-caafd1015e4e" />





