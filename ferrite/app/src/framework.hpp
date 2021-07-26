#pragma once

#include <memory>

#include <record/analogArrayIO.hpp>

void framework_init_device();

[[nodiscard]]
std::unique_ptr<AaoHandler> framework_record_init_dac(Aao &record);
