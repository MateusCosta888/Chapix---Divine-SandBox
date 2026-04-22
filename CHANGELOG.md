# Changelog - ChapiX: Divine Sandbox

Todas as alterações notáveis neste projeto serão documentadas neste arquivo.

O formato é baseado em [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
e este projeto adere ao [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-04-23

### ➕ Adicionado
- **Sistema de Build Automático**: Criado `BuildRelease.bat` para gerar versões finais otimizadas e empacotadas.
- **Tutorial de Onboarding**: Sistema de notificações em Português para guiar o início da civilização (Colocação de humanos, fundação de vilas, economia).
- **Progressão por Milestones**: O poder do Dragão agora está bloqueado até que a civilização atinja 50 cidadãos e 5 soldados.
- **Surgimento de Criaturas em Fases**: Criaturas (Slimes e Javalis) agora surgem em ondas baseadas no tempo e crescimento da população, evitando o sufocamento inicial.

### 🛠️ Corrigido
- **IA de Movimentação (Stuck Fix)**: Implementado detector de travamento que reseta automaticamente humanos presos em grupos ou obstáculos.
- **Mira do Dragão**: Corrigido o ataque de fogo do Dragão que agora atinge corretamente a posição do alvo.
- **Estabilidade do Build**: Resolvidos diversos erros de compilação que ocorriam apenas em builds otimizados.
- **Superpopulação**: Corrigido o bug que causava explosão populacional infinita e queda de performance.

### ⚖️ Balanceamento
- **Reprodução Dinâmica**: O tempo de nascimento agora escala com o tamanho da cidade (Inicia em 25s, sobe até 60s).
- **Economia de Comida**: O custo de comida para novos cidadãos agora aumenta progressivamente conforme a vila cresce.
- **Prioridade de Combate**: Soldados agora priorizam ameaças (Dragão > Inimigos > Monstros) de forma inteligente.
- **Navegação**: Permitido que humanos caminhem em águas rasas (Shallow Ocean) para evitar travamentos nas margens.

### 🔧 Melhorado
- **Performance de Release**: Compilação otimizada com flags `-O3` e `-s` (tamanho final ~3.9 MB).
- **Experiência do Usuário**: Console de debug e logs técnicos desativados por padrão no modo Release.
- **Persistência**: Adicionados campos de estado (`birthTimer`, `center`) para garantir que o balanceamento seja mantido após carregar um jogo salvo.

---

*Nota: Use este arquivo para comunicar mudanças críticas entre a equipe de desenvolvimento.*
