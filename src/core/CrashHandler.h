#pragma once
#include <string>

// ============================================================================
// CRASH HANDLER - Sistema de captura e log de crashes
// ============================================================================
namespace CrashHandler {

// Inicializa os handlers de sinais (chamar no início do main)
void Init();

// Registra contexto do jogo para inclusão no crash log
void SetGameContext(const std::string &context);

// Adiciona informação extra ao contexto (acumula)
void AddContext(const std::string &key, const std::string &value);

// Limpa o contexto
void ClearContext();

} // namespace CrashHandler
