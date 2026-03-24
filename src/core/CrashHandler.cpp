#include "CrashHandler.h"
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// ============================================================================
// INTERNAL STATE
// ============================================================================
static std::string g_gameContext = "Unknown";
static std::map<std::string, std::string> g_contextMap;
static bool g_initialized = false;

// ============================================================================
// HELPER: Get current timestamp string
// ============================================================================
static std::string GetTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm_buf;
#ifdef _WIN32
  localtime_s(&tm_buf, &time);
#else
  localtime_r(&time, &tm_buf);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

// ============================================================================
// HELPER: Signal name
// ============================================================================
static const char *SignalName(int sig) {
  switch (sig) {
  case SIGSEGV:
    return "SIGSEGV (Segmentation Fault - Acesso a memoria invalida)";
  case SIGABRT:
    return "SIGABRT (Abort - Erro interno / assert falhou)";
  case SIGFPE:
    return "SIGFPE (Floating Point Exception - Divisao por zero)";
  case SIGILL:
    return "SIGILL (Illegal Instruction - Instrucao invalida)";
  case SIGTERM:
    return "SIGTERM (Termination Request)";
  default:
    return "UNKNOWN SIGNAL";
  }
}

// ============================================================================
// HELPER: Capture stack trace (Windows - raw addresses)
// ============================================================================
static std::string CaptureStackTrace() {
  std::ostringstream oss;

#ifdef _WIN32
  void *stack[64];
  USHORT frames = CaptureStackBackTrace(0, 64, stack, NULL);

  oss << "Stack Trace (" << frames << " frames):\n";
  for (USHORT i = 0; i < frames; i++) {
    oss << "  [" << i << "] 0x" << std::hex << (uintptr_t)stack[i] << std::dec
        << "\n";
  }
  oss << "\nDica: Use 'addr2line -e bin/ChapiX.exe 0xENDERECO' para resolver "
         "nomes de funcoes.\n";
#else
  oss << "Stack trace nao disponivel nesta plataforma.\n";
#endif

  return oss.str();
}

// ============================================================================
// HELPER: Get system info
// ============================================================================
static std::string GetSystemInfo() {
  std::ostringstream oss;

#ifdef _WIN32
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&memInfo);

  DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
  DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;

  oss << "=== INFORMACAO DO SISTEMA ===\n";
  oss << "RAM Total: " << (totalPhysMem / (1024 * 1024)) << " MB\n";
  oss << "RAM Usada: " << (physMemUsed / (1024 * 1024)) << " MB\n";
  oss << "RAM Disponivel: " << (memInfo.ullAvailPhys / (1024 * 1024))
      << " MB\n";
  oss << "Uso de Memoria: " << memInfo.dwMemoryLoad << "%\n";
#else
  oss << "Informacao do sistema nao disponivel.\n";
#endif

  return oss.str();
}

// ============================================================================
// CRASH HANDLER (Signal Handler)
// ============================================================================
static void CrashSignalHandler(int sig) {
  // Prevent recursive crashes
  signal(sig, SIG_DFL);

  // Build crash report
  std::ostringstream report;

  report
      << "================================================================\n";
  report << "              CHAPIX - CRASH REPORT\n";
  report
      << "================================================================\n";
  report << "\n";
  report << "Data/Hora: " << GetTimestamp() << "\n";
  report << "Sinal: " << SignalName(sig) << " (codigo: " << sig << ")\n";
  report << "\n";

  report << "=== CONTEXTO DO JOGO ===\n";
  report << "Estado: " << g_gameContext << "\n";
  for (const auto &pair : g_contextMap) {
    report << pair.first << ": " << pair.second << "\n";
  }
  report << "\n";

  report << GetSystemInfo() << "\n";

  report << "=== STACK TRACE ===\n";
  report << CaptureStackTrace() << "\n";

  report << "=== DICAS PARA DEBUG ===\n";
  report << "1. Se SIGSEGV: Provavelmente acessou um ponteiro nulo ou memoria "
            "ja liberada.\n";
  report << "   - Verifique se GetCitizen/GetCity retorna nullptr antes de "
            "usar.\n";
  report << "   - Verifique bounds de arrays e vetores.\n";
  report << "2. Se SIGABRT: Um assert() falhou ou houve corrupcao de "
            "memoria.\n";
  report << "   - Verifique se ha double-free ou buffer overflow.\n";
  report << "3. Se SIGFPE: Divisao por zero em algum calculo.\n";
  report << "   - Verifique divisoes onde o divisor pode ser 0.\n";
  report << "\n";

  // Write to file
  std::string filename = "crash_log.txt";
  std::ofstream file(filename, std::ios::app);
  if (file.is_open()) {
    file << report.str();
    file << "\n========== FIM DO RELATORIO ==========\n\n";
    file.close();
  }

  // Also print to stderr
  fprintf(stderr, "\n\n!!! CRASH DETECTADO !!!\n");
  fprintf(stderr, "Sinal: %s\n", SignalName(sig));
  fprintf(stderr, "Contexto: %s\n", g_gameContext.c_str());
  fprintf(stderr, "Log completo salvo em: %s\n\n", filename.c_str());

  // Re-raise signal to get default behavior (core dump if configured)
  raise(sig);
}

// ============================================================================
// WINDOWS SEH HANDLER (Structured Exception Handling)
// ============================================================================
#ifdef _WIN32
static LONG WINAPI WindowsExceptionHandler(EXCEPTION_POINTERS *exInfo) {
  std::ostringstream report;

  report
      << "================================================================\n";
  report << "              CHAPIX - CRASH REPORT (SEH)\n";
  report
      << "================================================================\n";
  report << "\n";
  report << "Data/Hora: " << GetTimestamp() << "\n";

  DWORD code = exInfo->ExceptionRecord->ExceptionCode;
  const char *desc = "Desconhecido";
  switch (code) {
  case EXCEPTION_ACCESS_VIOLATION:
    desc = "ACCESS_VIOLATION (Acesso a memoria invalida)";
    break;
  case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    desc = "ARRAY_BOUNDS_EXCEEDED (Indice fora dos limites)";
    break;
  case EXCEPTION_STACK_OVERFLOW:
    desc = "STACK_OVERFLOW (Pilha estourada - recursao infinita?)";
    break;
  case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    desc = "FLT_DIVIDE_BY_ZERO (Divisao float por zero)";
    break;
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    desc = "INT_DIVIDE_BY_ZERO (Divisao inteira por zero)";
    break;
  case EXCEPTION_ILLEGAL_INSTRUCTION:
    desc = "ILLEGAL_INSTRUCTION (Instrucao ilegal)";
    break;
  case EXCEPTION_IN_PAGE_ERROR:
    desc = "IN_PAGE_ERROR (Erro de paginacao)";
    break;
  }

  report << "Excecao: " << desc << " (codigo: 0x" << std::hex << code
         << std::dec << ")\n";

  // Access violation details
  if (code == EXCEPTION_ACCESS_VIOLATION &&
      exInfo->ExceptionRecord->NumberParameters >= 2) {
    ULONG_PTR rwFlag = exInfo->ExceptionRecord->ExceptionInformation[0];
    ULONG_PTR address = exInfo->ExceptionRecord->ExceptionInformation[1];
    report << "Detalhes: Tentativa de "
           << (rwFlag == 0 ? "LEITURA" : (rwFlag == 1 ? "ESCRITA" : "EXECUCAO"))
           << " no endereco 0x" << std::hex << address << std::dec << "\n";
  }

  report << "Endereco da Excecao: 0x" << std::hex
         << (uintptr_t)exInfo->ExceptionRecord->ExceptionAddress << std::dec
         << "\n";
  report << "\n";

  report << "=== CONTEXTO DO JOGO ===\n";
  report << "Estado: " << g_gameContext << "\n";
  for (const auto &pair : g_contextMap) {
    report << pair.first << ": " << pair.second << "\n";
  }
  report << "\n";

  report << GetSystemInfo() << "\n";

  // Register info
  if (exInfo->ContextRecord) {
    report << "=== REGISTRADORES ===\n";
#ifdef _WIN64
    report << "RIP: 0x" << std::hex << exInfo->ContextRecord->Rip << "\n";
    report << "RSP: 0x" << exInfo->ContextRecord->Rsp << "\n";
    report << "RBP: 0x" << exInfo->ContextRecord->Rbp << "\n";
    report << "RAX: 0x" << exInfo->ContextRecord->Rax << "\n";
    report << "RBX: 0x" << exInfo->ContextRecord->Rbx << "\n";
    report << "RCX: 0x" << exInfo->ContextRecord->Rcx << "\n";
    report << "RDX: 0x" << exInfo->ContextRecord->Rdx << std::dec << "\n";
#else
    report << "EIP: 0x" << std::hex << exInfo->ContextRecord->Eip << "\n";
    report << "ESP: 0x" << exInfo->ContextRecord->Esp << "\n";
    report << "EBP: 0x" << exInfo->ContextRecord->Ebp << std::dec << "\n";
#endif
    report << "\n";
  }

  report << "=== STACK TRACE ===\n";
  report << CaptureStackTrace() << "\n";

  // Write to file
  std::string filename = "crash_log.txt";
  std::ofstream file(filename, std::ios::app);
  if (file.is_open()) {
    file << report.str();
    file << "\n========== FIM DO RELATORIO ==========\n\n";
    file.close();
  }

  // Show message box to user
  std::string msg = "O jogo crashou!\n\n";
  msg += "Excecao: " + std::string(desc) + "\n";
  msg += "Contexto: " + g_gameContext + "\n\n";
  msg += "Um log detalhado foi salvo em:\n" + filename + "\n\n";
  msg += "Por favor, envie este arquivo para ajudar a corrigir o bug.";
  MessageBoxA(NULL, msg.c_str(), "ChapiX - Crash!", MB_OK | MB_ICONERROR);

  return EXCEPTION_EXECUTE_HANDLER;
}
#endif

// ============================================================================
// PUBLIC API
// ============================================================================
void CrashHandler::Init() {
  if (g_initialized)
    return;
  g_initialized = true;

  // Register signal handlers
  signal(SIGSEGV, CrashSignalHandler);
  signal(SIGABRT, CrashSignalHandler);
  signal(SIGFPE, CrashSignalHandler);
  signal(SIGILL, CrashSignalHandler);
  signal(SIGTERM, CrashSignalHandler);

#ifdef _WIN32
  // Register Windows SEH handler (catches more than signals)
  SetUnhandledExceptionFilter(WindowsExceptionHandler);
#endif

  // Log startup
  std::ofstream file("crash_log.txt", std::ios::app);
  if (file.is_open()) {
    file << "=== SESSAO INICIADA: " << GetTimestamp() << " ===\n";
    file.close();
  }
}

void CrashHandler::SetGameContext(const std::string &context) {
  g_gameContext = context;
}

void CrashHandler::AddContext(const std::string &key,
                              const std::string &value) {
  g_contextMap[key] = value;
}

void CrashHandler::ClearContext() {
  g_contextMap.clear();
  g_gameContext = "Unknown";
}
