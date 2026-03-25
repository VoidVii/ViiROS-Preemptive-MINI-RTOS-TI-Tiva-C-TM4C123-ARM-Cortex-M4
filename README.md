# Preemptive Kernel ViiROS auf ARM Cortex-M4 (TM4C123GH6PM)
Präemtives Mini-RTOS (Real-Time Operating System) für ARM Cortex-M.
Mit klassischem ARM Cortex-M Context Switch über PendSV, fabrizierten Thread-Stacks, zeit- u. prioritätbasiertem Scheduling über SysTick. 

## Tiva C Series LaunchPad TM4C123GXL (TM4C123GH6PM)
<img width="400" height="360" alt="grafik" src="https://github.com/user-attachments/assets/1c4a9de0-9fca-4949-b539-c2568f10de35" />
Quelle: https://www.ti.com/tool/EK-TM4C123GXL

## Überblick
Das System nutzt den SysTick als Zeitbasis von 1 ms. Sobald ViiROS die Kontrolle über das System hat, wird der MSP (Main Stack Pointer) nur noch für das Interrupt-Handling verwendet – alle Threads laufen auf dem PSP (Process Stack Pointer).

## Features:

- Thread-System
	- Thread Control Blocks (TCB)
	- Stack pro Thread
	- fabrizierter Inital-Stack (aufgebaut als wäre Thread vom Interrupt unterbochen)
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
	- PendSV als Context Swtich Interrupt in assambly
	- Manuelles Sichern/Laden der Calle Save Register (R4-R11) auf/vom PSP
	- Manuelles PSP setzen

- CPU-Modus-Handling:
	- Handler Mode u. Thread Mode
	- MSP (Main Stack Pointer) für Interrupts
	- PSP (Process Stack pointer) für Threads
	- Spacial Register CONTROL (= 0x02 -> SPSEL; Umschalten von MSP -> PSP)
	- EXC_RETURN 0xFFFFFFFD (Scheduler returns from Interrupt into thread)

## Periodischer Ablauf:

	SysTick (1 ms)
	      |
	BlockWatch (dekrementiert blocktime, set/clear readyMask/blockedMask)
	      │
	Scheduler (LOG2(readyMask) → höchste Prio)
	      │
	PendSV (Context Switch)
	      │
	Thread läuft auf PSP

    
## Scheduler
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
### Die Funktionsweise des Schedulers besteht aus zwei simplen Abfragen:
- wenn readyMask 0= 0, so soll der Idle-Thread als nächstes laufen
- wenn readyMask != 0, finde mit LOG2 = 32 - CLZ(readyMask) nächst höchste ready Prio 
- setze das Pendingbit von PendSV um den Context Switch aus zulösen

## Context Switch mit PendSV

	__stackless void PendSV_Handler(void)
	{
	  __asm volatile(
	  
	        /* __disable_irq();*/
	       " CPSID     i                    \n"
	         
	        /* if (ViiROS_current != 0) */
	       " LDR.N     R2, =ViiROS_current   \n"    // R2 = current 
	       " LDR       R0, [R2]              \n"    // R0 = current-TCB
	       " CBZ       R0, PendSV_first_run  \n"    // R0 == 0? -> restore
	
	       /* save R4 - R11 on stack */  
	       " MRS       R1, PSP               \n"    // R1 = PSP
	         
	       /* Store Multiple, Decrement Before */
	       " STMDB     R1!, {R4-R11}         \n"    // R1 = new PSP at R4
	       " STR       R1, [R0]              \n"    // TCB->sp = R1
	         
	       /* sp = ViiROS_next->sp; */
	       "PendSV_restore:                  \n"
	       " LDR       R0, =ViiROS_next      \n"     // R0 = next
	       " LDR       R1, [R0]              \n"     // R1 = next-TCB
	       " LDR       R0, [R1]              \n"     // R0 = next-TCB->sp
	       /* Load multiple, increment after */   
	       " LDMIA     R0!, {R4-R11}         \n"     // Load R4 - R11, new sp at R0
	       " MSR       PSP, R0               \n"     // PSP = R0 (SP)
	        
	         
	        /* ViiROS_current = ViiROS_next; */
	       " STR       R1, [R2]             \n"
	         
	        /* __enable_irq(); */
	       " CPSIE     i                    \n"
	       " BX        LR                   \n" 
	
	       /*#### FIRST pendSV RUN ######### */  
	       "PendSV_first_run:               \n"
	       
	       " MOV       R3, #0x02            \n" 
	       " MSR       CONTROL, R3          \n" 
	       " ISB                            \n" 
	       " LDR       LR, =0xFFFFFFFD      \n" /* << bug fix wihtout interrupt returns to main */
	       " B         PendSV_restore       \n"  
	  );
	}
### PendSV Funktionsweise:
#### Beim ersten PendSV in System:
- Current-Thread = NULL -> springt zu PendSV_first_run:
  	- Setze Special Register CONTROL = 0x02 (SPSEL = 1 => Bit 1) ==> sagt der CPU "Benutze PSP" (Process Stack Pointer)
  	- Setze LR = 0xFFFFFFFD (EXC_RETURN) ==> Spring aus dem Interrupt und nutze PSP
  	- ISB ==> notwendig um Änderungen zu übernehmen
  	- Spring zu  PendSV_restore
  		- Lade ViiROS_next => Thread_TCB => Thread->sp in R0
  	 	- LDMIA     R0!, {R4-R11} (load multible and increment after) => Lade R4 vom Stack in R4 und inkrementiere R0
  	  	- nach dem R11 geladen wurde zeigt der R0 im Register auf R0 in unserem Thread-Sack
  	  	- MSR       PSP, R0  (move general purpose register into special register)	Setzte nun PSP auf den Wert von r0
  	  	- PSP ist nun auf R0 im Stack ausgerichtet und bereit für den Sprung aus dem Interrupt
  	  	- Nach BX LR werden die Hardware Register (Caller Save) automatisch geladen 

**Wichtig:** **CONTROL = 0x02** und **LR = 0xFFFFFFFD** in Verbindung sind wichtig. Da der erste PendSV-Interrupt-Aufruf aus main() erfolgt steht zu nächst der falsche Wert im LR-Register!!! Wird der Wert in LR nicht auf 0xFFFFFFFD gesetzt so kehrt der PendSV-Interrupt wieder zurück zu main.c und beendet das Programm. 

### Normaler PendSV-Durchlauf
Nach dem aller ersten Durchlauf von PendSV wird der Brench PendSV_first_run nie wieder aufgerufen! Jetzt wird immer der normale Durchlauf für den Context Switch durch gelaufen.
- Speichere den Kontext des aktuellen Threads
	- MRS       R1, PSP (move special register intro general purpose register) => lade PSP und R1
 	- STMDB     R1!, {R4-R11} (store multible and decrement before) => dekrementiere R1 und speichere R11 auf PSP-Stack -> --R1 save R10
  	- Reihenfolge ist hier umgedreht R11 -> R4  (beim Laden R4 - R11)
	- STR       R1, [R0]  => Speichere R1 (PSP) in [R0] (Thread->SP) **R0 = Thread_TCB => [Thread_TCN] = Thread->SP (erstes Atrribut im TCB)
	- Somit sind R4 - R11 auf dem Stack von Current-Thread und der PSP im Current-Thread->SP gespeichert!!
- Lade den nächsten Thread
	- Lad in R0 den Thread->SP
 	- LDMIA     R0!, {R4-R11} => Lade R4 vom Stack in R4 und inkrementiere R0
  	- Reihenfolge beim Laden ist R4 - R11
  	- R0 zeigt nach dem Laden auf R0 auf dem PSP-Stack und ist bereit zum Laden der Hardware Register
  	- MSR       PSP, R0 => Setze PSP auf neune R0
  	- STR       R1, [R2] => setze ViiROS_current auf ViiROS_next
 
			LOW ADDRESS
			*                --------------
			*        --------R4 <-- sp (Process Stack Pointer) vor den Laden
			*                R5
			*                R6
			*   Software stk R7
			*                R8
			*                R9
			*                R10
			*       ---------R11
			*                R0 <-- sp (Process Stack Pointer) nach dem den Laden
			*                R1
			*                R2
			*   Hardware stk R3
			*                R12
			*                LR
			*                PC
			*       --------xPSR
			*                --------------
			*                HIGH ADDRESS



## Logic Analyzer View - Folgt noch um die präemptive Wirkungsweise zu beweisen!
### PulseView software from sigrok.org 


## Architektur

	
	┌─────────────────────────────────────────────────────────────────────────────┐
	│                              ViiROS KERNEL                                  │
	├─────────────────────────────────────────────────────────────────────────────┤
	│                                                                             │
	│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐          │
	│  │   main()        │    │   SysTick_IRQ   │    │   PendSV_IRQ    │          │
	│  │                 │    │   (1ms)         │    │   (Context Sw)  │          │
	│  │ ViiROS_Init()   │    │                 │    │                 │          │
	│  │ ViiROS_Run()    │    │ ViiROS_Block    │    │ ┌─────────────┐ │          │
	│  │ ViiROS_lastInit │    │    Watch()      │    │ │save R4-R11  │ │          │
	│  └────────┬────────┘    │ ViiROS_Sched    │    │ │restore R4-11│ │          │
	│           │             │    uler()       │    │ │set PSP      │ │          │
	│           │             └────────┬────────┘    │ │switch SP    │ │          │
	│           │                      │             │ └─────────────┘ │          │
	│           ▼                      ▼             └────────┬────────┘          │
	│  ┌────────────────────────────────────────────────────────────────┐         │
	│  │                     THREAD MANAGEMENT                          │         │
	│  ├────────────────────────────────────────────────────────────────┤         │
	│  │                                                                │         │
	│  │  ┌─────────────────────────────────────────────────────┐       │         │
	│  │  │              Ready Queue (Priority based)           │       │         │
	│  │  │                                                     │       │         │
	│  │  │  readyMask = 0b...100...010...0                     │       │         │
	│  │  │                                                     │       │         │
	│  │  │  Prio 31 │ Prio 30 │ ... │ Prio 2 │ Prio 1 │ Idle   │       │         │
	│  │  │  ┌─────┐ │ ┌─────┐ │     │ ┌─────┐ │ ┌─────┐ │┌───┐ │       │         │
	│  │  │  │Threa│ │ │Threa│ │ ... │ │Threa│ │ │Threa│ ││Idl│ │       │         │
	│  │  │  └─────┘ │ └─────┘ │     │ └─────┘ │ └─────┘ │└───┘ │       │         │
	│  │  └─────────────────────────────────────────────────────┘       │         │
	│  │                                                                │         │
	│  │  ┌─────────────────────────────────────────────────────┐       │         │
	│  │  │            Blocked Queue (Time based)               │       │         │
	│  │  │                                                     │       │         │
	│  │  │  blockedMask = 0b...010...0                         │       │         │
	│  │  │                                                     │       │         │
	│  │  │  Thread A (blocktime=5ms)                           │       │         │
	│  │  │  Thread B (blocktime=3ms)                           │       │         │
	│  │  │  ...                                                │       │         │
	│  │  └─────────────────────────────────────────────────────┘       │         │
	│  └────────────────────────────────────────────────────────────────┘         │
	│                                                                             │
	└─────────────────────────────────────────────────────────────────────────────┘
	
	┌─────────────────────────────────────────────────────────────────────────────┐
	│                           THREAD CONTROL BLOCK (TCB)                        │
	├─────────────────────────────────────────────────────────────────────────────┤
	│                                                                             │
	│  ┌──────────────────────────────────────────────────────────────────┐       │
	│  │  ViiROS_Thread                                                   │       │
	│  │  ┌───────────────┐                                               │       │
	│  │  │ sp (uint32_t*)│ ────► Stack Pointer (zeigt auf R4 im Stack)   │       │
	│  │  ├───────────────┤                                               │       │
	│  │  │ priority      │                                               │       │
	│  │  │ (uint8_t)     │                                               │       │
	│  │  ├───────────────┤                                               │       │
	│  │  │ blocktime     │                                               │       │
	│  │  │ (uint32_t)    │                                               │       │
	│  │  └───────────────┘                                               │       │
	│  └──────────────────────────────────────────────────────────────────┘       │
	│                                                                             │
	└─────────────────────────────────────────────────────────────────────────────┘
	
	┌─────────────────────────────────────────────────────────────────────────────┐
	│                           THREAD STACK LAYOUT                               │
	├─────────────────────────────────────────────────────────────────────────────┤
	│                                                                             │
	│  ┌─────────────────────────────────────────────────────────────────┐        │
	│  │  HIGH ADDRESS                                                   │        │
	│  │  ┌─────────────────┐                                            │        │
	│  │  │      ...        │                                            │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │    xPSR         │  (0x21000000 - Thumb-Bit)                  │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      PC         │  (Thread Entry Point)                      │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      LR         │  (0xFFFFFFFD - EXC_RETURN)                 │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R12        │                                            │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R3         │                                            │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R2         │  Hardware Stack Frame                      │        │
	│  │  ├─────────────────┤  (Auto-saved by CPU on interrupt)          │        │
	│  │  │      R1         │                                            │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R0         │                                            │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R11        │ ◄──                                        │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R10        │                                            │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R9         │                                            │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R8         │  Software Stack Frame                      │        │
	│  │  ├─────────────────┤  (Manually saved by PendSV)                │        │
	│  │  │      R7         │                                            │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R6         │                                            │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R5         │                                            │        │
	│  │  ├─────────────────┤                                            │        │
	│  │  │      R4         │ ◄── PSP zeigt hierher                      │        │
	│  │  └─────────────────┘                                            │        │
	│  │  LOW ADDRESS                                                    │        │
	│  └─────────────────────────────────────────────────────────────────┘        │
	│                                                                             │
	└─────────────────────────────────────────────────────────────────────────────┘
	
	┌─────────────────────────────────────────────────────────────────────────────┐
	│                         CONTROL FLOW DIAGRAM                                │
	├─────────────────────────────────────────────────────────────────────────────┤
	│                                                                             │
	│   main()                                                                    │
	│     │                                                                       │
	│     ▼                                                                       │
	│   ViiROS_Init() 							      							  │
	│     │                                                                       │
	│     │  - Set PendSV priority (lowest)                                       │
	│     │  - Create Idle Thread                                                 │
	│     │  - ViiROS_current = NULL                                              │
	│     │                                                                       │
	│     ▼                                                                       │
	│   ViiROS_Run()                                                              │
	│     │                                                                       │
	│     │  - ViiROS_lastInit()                                                  │
	│     │  - ViiROS_Scheduler()                                                 │
	│     │         │                                                             │
	│     │         ├─► Find highest priority thread via LOG2(readyMask)          │
	│     │         │                                                             │
	│     │         └─► SCB->ICSR |= PENDSVSET ─────────────────────────┐         │
	│     │                                                             │         │
	│     │                                                             ▼         │
	│     │                                                   ┌──────────────────┐│
	│     │                                                   │   PendSV_Handler ││
	│     │                                                   │                  ││
	│     │                                                   │ current == NULL? ││
	│     │                                                   │   │              ││
	│     │                                                   │   ├─YES─────────┐││
	│     │                                                   │   │             │││
	│     │                                                   │   │ Set CONTROL │││
	│     │                                                   │   │ Set LR=FD   │││
	│     │                                                   │   │             │││
	│     │                                                   │   └─NO──────────┘││
	│     │                                                   │   │              ││
	│     │                                                   │ Save R4 - R11    ││
	│     │                                                   │   │              ││
	│     │                                                   │   ▼              ││
	│     │                                                   │ Restore R4-R11   ││
	│     │                                                   │ Set PSP          ││
	│     │                                                   │ BX LR            ││
	│     │                                                   └──────────────────┘│
	│     │                                                            │          │
	│     ▼                                                            ▼          │
	│   ┌─────────────────────────────────────────────────────────────────┐       │
	│   │                    THREAD EXECUTION                             │       │
	│   │                                                                 │       │
	│   │  ┌─────────────────────────────────────────────────────────┐    │       │
	│   │  │  Thread runs...                                         │    │       │
	│   │  │     │                                                   │    │       │
	│   │  │     ├─► ViiROS_BlockTime() ──► set blocktime            │    │       │
	│   │  │     │       │                  clear readyMask          │    │       │
	│   │  │     │       │                  set blockedMask          │    │	      │
	│   │  │     │       └─► ViiROS_Scheduler() ──► trigger PendSV   │    │       │
	│   │  │     │                                                   │    │       │
	│   │  │     └─► SysTick_Handler (every 1ms)                     │    │       │
	│   │  │              │                                          │    │       │
	│   │  │              ├─► ViiROS_BlockWatch()                    │    │       │
	│   │  │              │      │                                   │    │       │
	│   │  │              │      └─► decrement blocktime             │    │       │
	│   │  │              │           if blocktime == 0:             │    │       │
	│   │  │              │             readyMask |= bit             │    │       │
	│   │  │              │             blockedMask &= ~bit          │    │       │
	│   │  │              │                                          │    │       │
	│   │  │              └─► ViiROS_Scheduler() ──► trigger PendSV  │    │       │
	│   │  │                                                         │    │       │
	│   │  └─────────────────────────────────────────────────────────┘    │       │
	│   │                                                                 │       │
	│   │  Idle Thread (Prio 0)                                           │       │
	│   │     │                                                           │       │
	│   │     └─► __WFI() (Wait for Interrupt)                            │       │
	│   │                                                                 │       │
	│   └─────────────────────────────────────────────────────────────────┘       │
	│                                                                             │
	└─────────────────────────────────────────────────────────────────────────────┘
	
	┌─────────────────────────────────────────────────────────────────────────────┐
	│                         INTERRUPT PRIORITIES                                │
	├─────────────────────────────────────────────────────────────────────────────┤
	│                                                                             │
	│   Higher Priority                                                           │
	│        │                                                                    │
	│        ▼                                                                    │
	│   ┌─────────────────────────────────────────────────────────────────┐       │
	│   │  Other Interrupts (SysTick, GPIO, UART, etc.)                   │       │
	│   │  (Priorities 0x00 - 0xFE)                                       │       │
	│   └─────────────────────────────────────────────────────────────────┘       │
	│        │                                                                    │
	│        ▼                                                                    │
	│   ┌─────────────────────────────────────────────────────────────────┐       │
	│   │  PendSV (Priority 0xFF - lowest)                                │       │
	│   │  - Context switch only when no other interrupts pending         │       │
	│   └─────────────────────────────────────────────────────────────────┘       │
	│        │                                                                    │
	│        ▼                                                                    │
	│   Lower Priority                                                            │
	│                                                                             │
	└─────────────────────────────────────────────────────────────────────────────┘
	
	┌─────────────────────────────────────────────────────────────────────────────┐
	│                         PRIORITY MAPPING                                    │
	├─────────────────────────────────────────────────────────────────────────────┤
	│                                                                             │
	│   Priority    │ Bit in Mask   │ Description                                 │
	│   ────────────┼───────────────┼─────────────────────────────────────────────│
	│   0           │ (not in mask) │ Idle Thread (always ready if mask == 0)     │
	│   1           │ bit 0         │ Lowest user thread priority                 │
	│   2           │ bit 1         │                                             │
	│   ...         │ ...           │                                             │
	│   31          │ bit 30        │                                             │
	│   32          │ bit 31        │ Highest user thread priority                │
	│                                                                             │
	│   readyMask = (1 << (priority - 1))                                         │
	│                                                                             │
	│   Example: Priority 8 → bit 7 → readyMask |= (1 << 7)                       │
	│            Priority 18 → bit 17 → readyMask |= (1 << 17)                    │
	│                                                                             │
	│   Highest Priority Finding:                                                 │
	│   highPrio = LOG2(readyMask) = 32 - __CLZ(readyMask)                        │
	│                                                                             │
	└─────────────────────────────────────────────────────────────────────────────┘

# Allgemeine Hinweise zum Starten des Systems
ViiROS kann mit bis zu 32 Threads verschiedener Prioritäten arbeiten. 

Für die Threads muss der Thread (TCB), Stack mit Stackgröße, Thread-Handler in main.c deklariert und programmiert werden.
Dieses wurde für den Idle-Thread bereits in ViiROS.c hinterlegt. Der Idle-Thread wird mit ViiROS_Init() gestartet.
Das Starten der Threads erfolgt mit:

    ViiROS_Thread Red;
    static uint32_t stack_Red[80];

    void Red_Handler(void)
    {
      while(1){
       /* Code */
    }
  
    ViiROS_ThreadStart(&Red,
                         Red_Handler,
                         Priority,
                         stack_Red, sizeof(stack_Red));

Um den Wechsel von MSP auf PSP sicherzustellen muss der Current-Thread für den allerersten Context Switch mit NULL initialisiert werden:

    ViiROS_current = NULL;

Nach vollständiger Konfiguration und Initialisierung der Komponenten wird die Kontrolle über das System an ViiROS übergeben:

    ViiROS_Run()
  
**Wichtig:** ViiROS_Run() kehrt nie zurück – das System läuft von nun an im Thread-Modus auf PSP.
 
**Debugger-Warnung ignorieren:**  
"The stack pointer for stack 'CSTACK' ... is outside the stack range"  
Normal bei RTOS: Threads laufen auf PSP, nicht auf CSTACK.


# Präemptiver RTOS-Kernel für ARM Cortex-M4 (TM4C123)
- Entwicklung eines vollständigen präemptiven Echtzeitkernels von Grund auf in C und Assembler

- Bitmask-basiertes Scheduling mit O(1)-Prioritätssuche durch LOG2(x) = 32 - __CLZ(x)

- Blocking-Mechanismus (BlockTime/BlockWatch) für zeitgesteuerte Threads (1 ms Auflösung)

- Kontextwechsel in Assembler (PendSV) mit niedrigster Interrupt-Priorität – sichert/stellt r4-r11 und wechselt zwischen MSP und PSP

- Stack-Management nach AAPCS: 8-Byte-Alignment, Magic Numbers (0xCAFEBABE) zur Erkennung von Stack-Overflows

- Doxygen-Dokumentation

# Entwicklungshinweis
Dieses Projekt baut konzeptionell auf den Inhalten des **Modern Embedded Systems Programming Course** von **Miro Samec (Quantum Leaps)** auf, den ich intensiv studiert habe.  
Die dort vermittelten Prinzipien – insbesondere zu präemptivem Scheduling, PendSV, Stack-Management und RTOS-Interna – habe ich verstanden und in eine **eigene, eigenständige Implementierung** überführt.

Der Code ist keine 1:1-Umsetzung von Beispielen, sondern meine eigene Arbeit, in der ich die Konzepte angewendet, weiterentwickelt und an meine Anforderungen angepasst habe. Die Lehren aus diesen Bemühungen und die eigenständige Umsetzung ermöglichten es mir, die Themen nicht nur anzuwenden, sondern auch auf einer tieferen Ebene zu verstehen und zu verinnerlichen.

Das Projekt Preemptive scheduler ViiROS baut auf meinem vorherigen Projekt Cooperative scheduler auf.
Ziel des Projekts war es, die Funktionsweise und Besonderheiten eines preemptiven System zu erlernen und umzusetzen.

## Die größten Herausforderungen dabei waren:
### Stack-Korruption durch falsche Initialisierung von Current-Thread (ViiROS_current):
  - Problem: ViiROS_current = Idle-Thread vor erstem Context Switch
  - Folge:   PSP (Process Stack Pointer) im Idle-TCB wurde mit MSP (Main Stack Pointer) im PendSV_Handler überschrieben.
           Sobald der Idle-Thread an der Reihe war führte der MSP wieder zurück in main() und beendete das System.
  - Lösung:  ViiROS_current = NULL, somit wurde der Wechsel von MSP zu PSP sichergestellt
### Stack Overflow durch zu kleine Thread-Stacks:
  - Problem:  Stackgröße für die einzelnen Threads zu klein gewählt. Der Stack wuchs in den Bereich des Arrays der aktiven Threads (Active_Thread[]).
  - Folge:    Array-Einträge wurden überschrieben und gestartete Threads zerstört.
  - Lösung:   Zu kleine Größe wurde mit hilfe von Magic Numbers (0xCAFEBABE) erkannt und der Stackgröße schrittweise erhöht.
### Hard Faults durch nicht initialisierte GPIO-Pins
  - Problem:  Ein Thread hat auf nicht initialisierte GPIO Pins zugegriffen.
  - Folge:    Hard Fault, sobald der Thread auf die LED zugriff.
  - Lösung:   In main() die GPIO Konfiguration ergänzen.
### CSTACK-Warnung im Debugger (kein echter Fehler, aber verwirrend)
  - Problem:  IAR zeigte: "Stack pointer is outside stack range". Der Debugger zeigte den PSP (Process Stack Pointer) an, der außerhalb von CSTACK liegt.
  - Folge:    Verunsicherung, ob das System korrekt läuft.
  - Lösung:   Verstehen, dass ViiROS eigene Stacks erstellt und auf eigenen PSP läuft, der nicht im CSTACK liegt.



# Build info
- IAR Embedded Workbench (Arm)
- Tiva C Series LaunchPad TM4C123GXL

# Hardware
- TI TM4C123GH6PM (ARM Cortex-M4)
- On-board LEDs
- User switch
