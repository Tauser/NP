// ServiceManager — ciclo de vida dos serviços da aplicação (ARCHITECTURE §3).
//
// PROPRIEDADE: registro, start e tick pertencem SOMENTE à `app_loop`. A lista
// é selada no primeiro start, antes de qualquer task de serviço poder existir;
// por isso não precisa mutex nem alocação dinâmica para permanecer segura.
#pragma once

#include <cstddef>
#include <cstdint>

#include "utils/status.hpp"

namespace nova {
namespace core {

class IAppService {
public:
    virtual ~IAppService() = default;

    // Inicialização curta e repetível após falha: ServiceManager retoma no
    // primeiro que falhou, sem iniciar novamente os anteriores.
    virtual utils::Status start() = 0;

    // Chamado somente pela app_loop já inicializada. Operação longa deve ir
    // para sua task/fila própria; bloquear aqui atrasaria UI e intenções.
    virtual void tick(uint64_t now_ms) = 0;
};

class ServiceManager {
public:
    static constexpr size_t kMaxServices = 8;

    utils::Status register_service(IAppService& service);
    utils::Status start_all();
    void tick_all(uint64_t now_ms);

    size_t service_count() const { return service_count_; }
    bool is_started() const { return started_count_ == service_count_; }

private:
    IAppService* services_[kMaxServices] = {};
    size_t service_count_ = 0;
    size_t started_count_ = 0;
    bool sealed_ = false;
};

}  // namespace core
}  // namespace nova
