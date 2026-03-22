# Preemptive Scheduler ViiROS for ARM Cortex-M4
Ein minimaler, präemptiver Echtzeitkernel entwickelt auf dem Tiva C Series LaunchPad TM4C123GXL (TM4C123GH6PM - ARM Cortex-M4).

Entwickelt als Lernprojekt für RTOS-Konzepte unter Anwendung der erworbenen Kenntnisse aus Modern Embedded Systems Programming Course von Miro Samec (Quantum Leaps).

Das System nutzt den SysTick als Zeitbasis von 1 ms und läuft auf dem PSP (Process Stack Pointer) statt auf dem MSP (Main Stack Pointer).
Soabld ViiROS die Kontrolle über das System hat wird der MSP nicht mehr verwendet. 


## Tiva C Series LaunchPad TM4C123GXL (TM4C123GH6PM)
<img width="640" height="360" alt="grafik" src="https://github.com/user-attachments/assets/1c4a9de0-9fca-4949-b539-c2568f10de35" />
Quelle: https://www.ti.com/tool/EK-TM4C123GXL
    
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

## Periodischer Ablauf:

                    SysTick (1 ms)
                         │
                    BlockWatch
                    (dekrementiert blocktime,
                     setzt readyMask/blockedMask)
                         │
                    Scheduler
                    (LOG2(readyMask) → höchste Prio)
                         │
                    PendSV
                    (Context Switch)
                         │
                    Thread läuft auf PSP


## Logic Analyzer View - Folgt noch um die präemptive Wirkungsweise zu beweisen!


## Architektur

Das folgende Diagramm wurde aus meinem Code in `ViiROS.c` abgeleitet und mit Hilfe einer KI visualisiert.

    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                              MAIN                                           │
    │  SystemCoreClockUpdate → __disable_irq → SysTick_Init → GPIO_Init           │
    │  → ViiROS_Init (startet Idle-Thread, setzt ViiROS_current = NULL)           │
    │  → ViiROS_ThreadStart() → __enable_irq → ViiROS_Run()                       │
    └─────────────────────────────────────────────────────────────────────────────┘
                                          │
                                          ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                         ViiROS_Run()                                        │
    │   __disable_irq → ViiROS_lastInit() → ViiROS_Scheduler() → __enable_irq     │
    │  (kehrt nie zurück)                                                         │
    └─────────────────────────────────────────────────────────────────────────────┘
                                          │
                                          │ (SysTick läuft parallel, 1 ms Takt)
                                          ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                      SYSTICK (1 ms Timer)                                   │
    │  ┌─────────────────────────────────────────────────────────────────────┐    │
    │  │ BlockWatch()                                                        │    │
    │  │ - durchläuft blockierte Threads (blockedMask)                       │    │
    │  │ - dekrementiert thread->blocktime                                   │    │
    │  │ - wenn blocktime == 0: setzt readyMask, löscht blockedMask          │    │
    │  └─────────────────────────────────────────────────────────────────────┘    │
    │                                      │                                      │
    │                                      ▼                                      │
    │  ┌─────────────────────────────────────────────────────────────────────┐    │
    │  │ Scheduler()                                                         │    │
    │  │ - prüft readyMask                                                   │    │
    │  │ - falls readyMask == 0 → Idle-Thread (Prio 0)                       │    │
    │  │ - sonst: highPrio = LOG2(readyMask) → Active_Thread[highPrio]       │    │
    │  │ - wenn current != next: ViiROS_next = next → trigger PendSV         │    │
    │  └─────────────────────────────────────────────────────────────────────┘    │
    └─────────────────────────────────────────────────────────────────────────────┘
                                          │
                                          │ (trigger PendSV)
                                          ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                      PENDSV_HANDLER (Assembler)                             │
    │  ┌─────────────────────────────────────────────────────────────────────┐    │
    │  │ CPSID i          (Interrupts aus)                                   │    │
    │  │ if(ViiROS_current != NULL) {                                        │    │
    │  │     PUSH {r4-r11}                   (Callee-saved sichern)          │    │
    │  │     ViiROS_current->sp = SP         (Stack speichern)               │    │
    │  │ }                                                                   │    │
    │  │ SP = ViiROS_next->sp                (Stack des neuen Threads laden) │    │
    │  │ ViiROS_current = ViiROS_next        (aktuellen Thread aktualisieren)│    │
    │  │ POP {r4-r11}                        (Callee-saved wiederherstellen) │    │
    │  │ CPSIE i          (Interrupts an)                                    │    │
    │  │ BX LR            (Rückkehr zum neuen Thread)                        │    │
    │  └─────────────────────────────────────────────────────────────────────┘    │
    └─────────────────────────────────────────────────────────────────────────────┘
                                          │
                                          ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                           THREADS (auf PSP)                                 │
    │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐              │
    │  │ Red (Prio 9)    │  │ Blue (Prio 8)   │  │ Green (Prio 7)  │              │
    │  │ Blinkt alle     │  │ Blinkt mit      │  │ Taster-Polling  │              │
    │  │ 50/50 ms        │  │ Muster 50/50/   │  │ + Entprellung   │              │
    │  │                 │  │ 250/120 ms      │  │ + Flankenerk.   │              │
    │  └─────────────────┘  └─────────────────┘  └─────────────────┘              │
    │                                                                             │
    │  ┌─────────────────────────────────────────────────────────────────────┐    │
    │  │ Idle (Prio 0) – läuft nur wenn readyMask == 0                       │    │
    │  │ __WFI() – CPU schläft, wartet auf nächsten Interrupt                │    │
    │  └─────────────────────────────────────────────────────────────────────┘    │
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


# Entwicklungshinweis
Das Projekt Preemptive scheduler ViiROS baut auf meinem vorherigen Projekt Cooperative scheduler auf.
Ziel des Projekts war es, die Funktionsweise und Besonderheiten eines preemptiven System zu erlernen und umzusetzen.
Bei der Umsetzung habe ich meinen Wissen, dass ich im Modern Embedded Systems Programming Course von Miro Samec (Quantum Leaps) erwerben durfte, eingesetzt und vertieft.

## Die größten Herausforderungen dabei waren:
### Stack-Korruption durch falsche Initialiesrung von Current-Thread (ViiROS_curennt):
  - Problem: ViiROS_current = Idle-Thread vor erstem Context Switch
  - Folge:   PSP (Process Stack Pointer) im Idle-TCB wurde mit MSP (Main Stack Pointer) im PendSV_Handler überschrieben.
           Sobald der Idle-Thread an der Reihe war führte der MSP wieder zurück in main() und beendete das System.
  - Lösung:  ViiROS_current = NULL, somit wurde der Wechsel von MSP zu PSP sichergestellt
### Stack Overflow durch zu kleine Thread-Stacks:
  - Problem:  Stackgröße für die einzelnen Threads zu klein gewählt. Der Stack wuchs in den Bereich des Arrays der aktiven Threads (Active_Thread[]).
  - Folge:    Array-Einträge wurden überschrieben und gestartete Threads zerstört.
  - Lösung:   Zu kleine Größe wurde mit hilfe von Magic Numbers (0xCAFEBABE) erkannt und der Stackgröße Schrittweise erhöht.
### Hard Faults durch nicht initialisierte GPIO-Pins
  - Problem:  Ein Thread hat auf nicht initialisierte GPIO Pins zugegriffen.
  - Folge:    Hard Fault, sobald der Thread auf die LED zugriff.
  - Lösung:   In main() die GPIO Konfiguration ergänzen.
### CSTACK-Warnung im Debugger (kein echter Fehler, aber verwirrend)
  - Problem:  IAR zeigte: "Stack pointer is outside stack range". Der Debugger zeigte den PSP (Process Stack Pointer) an, der außerhalb von CSTACK liegt.
  - Folge:    Verunsicherung, ob das System korrekt läuft.
  - Lösung:   Verstehen, dass ViiROS eigene Stacks erstellt und auf eigenen PSP läuft, der nicht im CSTACK liegt.

## KI-Unterstützung:
- Informationssuche
- Wissensvermittlung & Verständnis
- Kritik & Bewertung von Lösungen
- Quiz & Verständnisprüfung
- Diskussion & Entscheidungsfindung
- Architekturdiagramm (aus meinem Code generiert)

**Der gesamte Code, das Debugging, die Architekturentscheidungen und die finale Implementierung stammen von mir.**



# Build info
- IAR Embedded Workbench (Arm)
- Tiva C Series LaunchPad TM4C123GXL

# Hardware
- TI TM4C123GH6PM (ARM Cortex-M4)
- On-board LEDs
- User switch
