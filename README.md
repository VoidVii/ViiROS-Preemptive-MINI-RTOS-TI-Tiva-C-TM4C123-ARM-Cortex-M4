# ViiROS – Präemptives MINI RTOS für ARM Cortex-M4 (TM4C123GL - TM4C123GH6PM)
Dieses Projekt umfasst ein eigenständig erarbeitetes Mini RTOS mit:
- präemptives Scheduling
- prioritätenbasierte Ausführung der Threads
- klassischem Context Switch über PendSV
- MSP für Interrupts (Handler Mode)
- PSP für Threads (Thread Mode)
- SysTick als 1ms Zeitbasis

## Features
- Thread-System
	- Thread Control Blocks (TCB)
	- Stack pro Thread
	- fabrizierter Initial-Stack (aufgebaut als wäre Thread vom Interrupt unterbrochen)
	- Parallele Ausführung (Multitasking-System)

- Kernel / Scheduler:
	- Präemtiver Scheduler
	- Prioritätenbasiert 
	- Bitmasken (32 Bit -> 32 Prioritäten) mit (32 - CLZ) (count leading zeros) -> O(1):
		- readyMask     0b01000000000000000000000100000000 (Prio 31, 9 ready)
		- blockedMask  	0b00000000000000000000000000100001 (Prio 6, 1 blocked)

- Timing & Blocking
	- SysTick 1ms Zeitbasis
	- ViiROS_BlockTime() -> Block Threads
	- BlockWatch() -> Blocktime Management, Unblock Threads

- Kontextwechsel (Context Switch):
	- PendSV als Context Swtich Interrupt in assembly
	- Manuelles Sichern/Laden der Calle Save Register (R4-R11) auf/vom PSP
	- Manuelles PSP setzen

- CPU-Modus-Handling:
	- Handler Mode u. Thread Mode
	- MSP (Main Stack Pointer) für Interrupts
	- PSP (Process Stack pointer) für Threads
	- Spacial Register CONTROL (= 0x02 -> SPSEL; Umschalten von MSP -> PSP)
	- EXC_RETURN 0xFFFFFFFD (Scheduler returns from Interrupt into thread)

## Quick Start
ViiROS kann mit bis zu 32 Threads verschiedener Prioritäten arbeiten. 

Für die Threads muss:
- Thread (TCB)
- Stack mit Stackgröße
- Thread-Handler
 
in main.c deklariert und programmiert werden.

Dieses wurde für den **Idle-Thread** bereits in ViiROS.c hinterlegt. Der Idle-Thread wird mit ViiROS_Init() gestartet.
Das Starten der Threads erfolgt mit:

    ViiROS_Thread Red; /* Thread_TCB erstellen */
    static uint32_t stack_Red[80]; /* Stack mit Stackgröße initialisieren */
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

Um den **Wechsel von MSP auf PSP** sicherzustellen muss der Current-Thread für den allerersten Context Switch mit NULL initialisiert werden:

    ViiROS_current = NULL; /* wird in ViiROS_Init() gesetzt '/

Nach vollständiger Konfiguration und Initialisierung der Komponenten wird die Kontrolle über das System an ViiROS übergeben:

    ViiROS_Run()
  
**Wichtig:** ViiROS_Run() kehrt nie zurück – das System läuft von nun an im Thread-Modus auf PSP.


## Projektstruktur
	Datei:			Beschreibung:
	ViiROS.c/h		Kernel, Scheduler, Blocking
	SysTick.c/h		Zeitbasis (1ms)
	GPIO.c/h		LED, Taster 
	main.c			Beispiel-Threads


## Hardware & Toolchain
- IAR Embedded Workbench (Arm)
- Tiva C Series LaunchPad TM4C123GXL
- TI TM4C123GH6PM (ARM Cortex-M4)
- On-board LEDs
- User switch

## Wie es funktioniert
Das System führt nach dem Starten der einzelnen Threads mit **ViiROS_RUN()** gleich zu Beginn nach dem **PendSV** den Thread mit der **höchsten Priorität** in der **readyMask** aus. 
Zuvor wird durch den Aufruf des Schedulers der **nächste Thread to run** ausgewählt und der PendSV getriggert. Bei dem ersten Context Switch im PendSv wird das System von MSP auf PSP umgestellt. Sicher gestellt wird dies durch die Tatsache das noch kein Thread ausgeführt wurde und der Current-Thread ein NULL-Pointer ist. Bei dem aller ersten PendSV wird einmalig das Special Register CONTROL = 0x02 und der LR = 0xFFFFFFFD gesetzt. Beides ist erforderlich damit der Wechsel von MSP auf PSP gelingt und die Threads auf dem PSP laufen.

### Periodischer Ablauf

	SysTick (1 ms) [höchste Interrupt-Prio]
	      |
	BlockWatch (dekrementiert blocktime, set/clear readyMask/blockedMask)
	      │
	Scheduler (LOG2(readyMask) → höchste Prio)
	      │
	PendSV (Context Switch) [niedrigste Interrupt-Prio]
	      │
	Thread läuft auf PSP
	
### Scheduler
	   void ViiROS_Scheduler(void)
	    {
	      ViiROS_Thread *current = ViiROS_current;
	      ViiROS_Thread *next;
	      
	      if(ViiROS_readyMask == 0U) /**< no thread ready = idle */
	      {
	        next = Active_Thread[0];
	      }
	      else 
	      {
	        uint32_t highPrio = LOG2(ViiROS_readyMask); /**< highest ready prio */
	        next = Active_Thread[highPrio];
	      }
	      
	      if(current != next)
	      {
	        ViiROS_next = next;
	        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; /**<trigger PendSV for context switch */
	      } 
	    }
#### Die Funktionsweise des Schedulers besteht aus zwei simplen Abfragen:
- wenn readyMask 0= 0, so soll der Idle-Thread als nächstes laufen
- wenn readyMask != 0, finde mit LOG2 = 32 - CLZ(readyMask)- nächst höchste ready Prio 
- setze das Pendingbit von PendSV um den Context Switch auszulösen


### Context Switch (PendSV)
#### Beim ersten PendSV in System:
- Current-Thread = NULL -> springt zu PendSV_first_run:
  	- Setze Special Register CONTROL = 0x02 (SPSEL = 1 => Bit 1) ==> sagt der CPU "Benutze PSP" (Process Stack Pointer)
  	- Setze LR = 0xFFFFFFFD (EXC_RETURN) ==> Spring aus dem Interrupt und nutze PSP
  	- ISB ==> notwendig um Änderungen zu übernehmen

**Wichtig:** **CONTROL = 0x02** und **LR = 0xFFFFFFFD** in Verbindung sind wichtig. Da der erste PendSV-Interrupt-Aufruf aus main() erfolgt steht zu nächst der falsche Wert im LR-Register!!! Wird der Wert in LR nicht auf 0xFFFFFFFD gesetzt so kehrt der PendSV-Interrupt wieder zurück zu main.c und beendet das Programm. 

#### Normaler PendSV-Durchlauf
Nach erstem PendSV wird der Branch **PendSV_first_run nie wieder aufgerufen!** Jetzt wird immer der **normale Context Switch** durchlaufen.
- Speichere den Kontext des aktuellen Threads
	- MRS       R1, PSP (move special register into general purpose register) => lade PSP und R1
 	- STMDB     R1!, {R4-R11} (store multible, decrement before) => dekrementiere R1 und speichere R11 auf PSP-Stack -> --R1 save R10
  	- Reihenfolge ist hier R11 -> R4  (beim Laden R4 - R11)
	- STR       R1, [R0]  => Speichere R1 (PSP) in [R0] (Thread->SP) **R0 = Thread_TCB => [Thread_TCN] = Thread->SP (erstes Atrribut im TCB)
	- Speichere PSP im Thread->SP

- Lade den nächsten Thread
	- Lad in R0 den Thread->SP (Darin ist PSP vom nächsten Thread gespeichert)
 	- LDMIA     R0!, {R4-R11} => Lade R4 vom Stack in R4 und inkrementiere R0 (Reihenfolge beim Laden ist R4 - R11)
  	- R0 zeigt nach dem Laden auf **R0 auf dem PSP-Stack** und ist bereit zum Laden der Hardware Register
  	- MSR       PSP, R0 => Setze PSP auf neuen R0
  	- STR       R1, [R2] => setze ViiROS_current auf ViiROS_next

### Thread-Zustände & Blocking

Die Threads haben zwei Zustände **Ready** und **Blocked**. Beide werden in jeweils einer 32-Bit Bitmaske repräsentiert. 
Der **Index** der Bits steht für die **Priorität** der Threads:

*                  Bit:         31 30 29 ... 5 4 3 2 1 0 <-- (priority - 1)
*                  Value:        0  0  0 ... 0 1 0 1 0 1 

- "Ready" - Threads -> readyMask
- "Blocked" - Threads -> blockedMask

Zum Scannen der Masken wird die CLZ() [Count leading zeros] auf ARM verwendet und ermöglicht das Auslesen in einem Durchlauf.


## Herausforderungen & Lösungen
### Stack-Korruption durch falsche Initialisierung von Current-Thread (ViiROS_current):
  - Problem: ViiROS_current = Idle-Thread vor erstem Context Switch
  - Folge:   PSP (Process Stack Pointer) im Idle-TCB wurde mit MSP (Main Stack Pointer) im PendSV_Handler überschrieben.
           Sobald der Idle-Thread an der Reihe war führte der MSP wieder zurück in main() und beendete das System.
  - Lösung:  ViiROS_current = NULL, somit wurde der Wechsel von MSP zu PSP sichergestellt

### Stack Overflow durch zu kleine Thread-Stacks:
  - Problem:  Stackgröße für die einzelnen Threads zu klein gewählt. Der Stack wuchs in den Bereich des Arrays der aktiven Threads (Active_Thread[]).
  - Folge:    Array-Einträge wurden überschrieben und gestartete Threads zerstört.
  - Lösung:   Zu kleine Größe wurde mit Hilfe von Magic Numbers (0xCAFEBABE) erkannt und der Stackgröße schrittweise erhöht.

### Hard Faults durch nicht initialisierte GPIO-Pins
  - Problem:  Ein Thread hat auf nicht initialisierte GPIO Pins zugegriffen.
  - Folge:    Hard Fault, sobald der Thread auf die LED zugriff.
  - Lösung:   In main() die GPIO Konfiguration ergänzen.

## Architekturdiagramm

	┌────────────────────────────────────────────┐
	│              ViiROS KERNEL                 │
	├────────────────────────────────────────────┤
	│                                            │
	│   SYSTICK ──► SCHEDULER ──► PENDSV         │
	│   (1ms)        (O(1))       (Context Sw)   │
	│                                 │          │
	│                                 ▼          │
	│   ┌─────────────────────────────────┐      │
	│   │      THREAD MANAGEMENT          │      │
	│   │  ┌───────────┬───────────┐      │      │
	│   │  │  READY    │  BLOCKED  │      │      │
	│   │  │ (Bitmask) │(BlockTime)│      │      │
	│   │  └───────────┴───────────┘      │      │
	│   └─────────────────────────────────┘      │
	│                    │                       │
	│                    ▼                       │
	│   ┌─────────────────────────────────┐      │
	│   │      HARDWARE (TM4C123GL)       │      │
	│   │   LED   │   Switch   │ SysTick  │      │
	│   └─────────────────────────────────┘      │
	└────────────────────────────────────────────┘
## Proof of Concept

## Lernhintergrund
Dieses Projekt baut konzeptionell auf den Inhalten des **Modern Embedded Systems Programming Course** von **Miro Samec (Quantum Leaps)** auf, den ich intensiv studiert habe.  Die dort vermittelten Prinzipien – insbesondere zu präemptivem Scheduling, PendSV, Stack-Management und RTOS-Interna – habe ich verstanden und in eine **eigene, eigenständige Implementierung** überführt.

Der Code ist keine 1:1-Umsetzung von Beispielen, sondern meine eigene Arbeit, in der ich die Konzepte angewendet, weiterentwickelt und an meine Anforderungen angepasst habe. Die Lehren aus diesen Bemühungen und die eigenständige Umsetzung ermöglichten es mir, die Themen nicht nur anzuwenden, sondern auch auf einer tieferen Ebene zu verstehen und zu verinnerlichen.

Das Projekt Preemptive scheduler ViiROS baut auf meinem vorherigen Projekt Cooperative scheduler auf.
