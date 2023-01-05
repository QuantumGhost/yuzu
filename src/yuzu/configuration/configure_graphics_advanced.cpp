// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/settings.h"
#include "core/core.h"
#include "ui_configure_graphics_advanced.h"
#include "yuzu/configuration/configuration_shared.h"
#include "yuzu/configuration/configure_graphics_advanced.h"

ConfigureGraphicsAdvanced::ConfigureGraphicsAdvanced(const Core::System& system_, QWidget* parent)
    : QWidget(parent), ui{std::make_unique<Ui::ConfigureGraphicsAdvanced>()}, system{system_} {

    ui->setupUi(this);

    SetupPerGameUI();

    SetConfiguration();
}

ConfigureGraphicsAdvanced::~ConfigureGraphicsAdvanced() = default;

void ConfigureGraphicsAdvanced::SetConfiguration() {
    const bool runtime_lock = !system.IsPoweredOn();
    ui->use_vsync->setEnabled(runtime_lock);
    ui->use_asynchronous_shaders->setEnabled(runtime_lock);
    ui->anisotropic_filtering_combobox->setEnabled(runtime_lock);

    ui->renderer_force_max_clock->setChecked(Settings::values.renderer_force_max_clock.GetValue());
    ui->use_vsync->setChecked(Settings::values.use_vsync.GetValue());
    ui->use_asynchronous_shaders->setChecked(Settings::values.use_asynchronous_shaders.GetValue());
    ui->use_fast_gpu_time->setChecked(Settings::values.use_fast_gpu_time.GetValue());
    ui->use_pessimistic_flushes->setChecked(Settings::values.use_pessimistic_flushes.GetValue());
    ui->use_vulkan_driver_pipeline_cache->setChecked(
        Settings::values.use_vulkan_driver_pipeline_cache.GetValue());

    if (Settings::IsConfiguringGlobal()) {
        ui->gpu_accuracy->setCurrentIndex(
            static_cast<int>(Settings::values.gpu_accuracy.GetValue()));
        ui->anisotropic_filtering_combobox->setCurrentIndex(
            Settings::values.max_anisotropy.GetValue());
    } else {
        ConfigurationShared::SetPerGameSetting(ui->gpu_accuracy, &Settings::values.gpu_accuracy);
        ConfigurationShared::SetPerGameSetting(ui->renderer_force_max_clock,
                                               &Settings::values.renderer_force_max_clock);
        ConfigurationShared::SetPerGameSetting(ui->anisotropic_filtering_combobox,
                                               &Settings::values.max_anisotropy);
        ConfigurationShared::SetHighlight(ui->label_gpu_accuracy,
                                          !Settings::values.gpu_accuracy.UsingGlobal());
        ConfigurationShared::SetHighlight(ui->af_label,
                                          !Settings::values.max_anisotropy.UsingGlobal());
    }
}

void ConfigureGraphicsAdvanced::ApplyConfiguration() {
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.gpu_accuracy, ui->gpu_accuracy);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.renderer_force_max_clock,
                                             ui->renderer_force_max_clock,
                                             renderer_force_max_clock);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.max_anisotropy,
                                             ui->anisotropic_filtering_combobox);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_vsync, ui->use_vsync, use_vsync);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_asynchronous_shaders,
                                             ui->use_asynchronous_shaders,
                                             use_asynchronous_shaders);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_fast_gpu_time,
                                             ui->use_fast_gpu_time, use_fast_gpu_time);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_pessimistic_flushes,
                                             ui->use_pessimistic_flushes, use_pessimistic_flushes);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_vulkan_driver_pipeline_cache,
                                             ui->use_vulkan_driver_pipeline_cache,
                                             use_vulkan_driver_pipeline_cache);
}

void ConfigureGraphicsAdvanced::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureGraphicsAdvanced::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureGraphicsAdvanced::SetupPerGameUI() {
    // Disable if not global (only happens during game)
    if (Settings::IsConfiguringGlobal()) {
        ui->gpu_accuracy->setEnabled(Settings::values.gpu_accuracy.UsingGlobal());
        ui->renderer_force_max_clock->setEnabled(
            Settings::values.renderer_force_max_clock.UsingGlobal());
        ui->use_vsync->setEnabled(Settings::values.use_vsync.UsingGlobal());
        ui->use_asynchronous_shaders->setEnabled(
            Settings::values.use_asynchronous_shaders.UsingGlobal());
        ui->use_fast_gpu_time->setEnabled(Settings::values.use_fast_gpu_time.UsingGlobal());
        ui->use_pessimistic_flushes->setEnabled(
            Settings::values.use_pessimistic_flushes.UsingGlobal());
        ui->use_vulkan_driver_pipeline_cache->setEnabled(
            Settings::values.use_vulkan_driver_pipeline_cache.UsingGlobal());
        ui->anisotropic_filtering_combobox->setEnabled(
            Settings::values.max_anisotropy.UsingGlobal());

        return;
    }

    ConfigurationShared::SetColoredTristate(ui->renderer_force_max_clock,
                                            Settings::values.renderer_force_max_clock,
                                            renderer_force_max_clock);
    ConfigurationShared::SetColoredTristate(ui->use_vsync, Settings::values.use_vsync, use_vsync);
    ConfigurationShared::SetColoredTristate(ui->use_asynchronous_shaders,
                                            Settings::values.use_asynchronous_shaders,
                                            use_asynchronous_shaders);
    ConfigurationShared::SetColoredTristate(ui->use_fast_gpu_time,
                                            Settings::values.use_fast_gpu_time, use_fast_gpu_time);
    ConfigurationShared::SetColoredTristate(ui->use_pessimistic_flushes,
                                            Settings::values.use_pessimistic_flushes,
                                            use_pessimistic_flushes);
    ConfigurationShared::SetColoredTristate(ui->use_vulkan_driver_pipeline_cache,
                                            Settings::values.use_vulkan_driver_pipeline_cache,
                                            use_vulkan_driver_pipeline_cache);
    ConfigurationShared::SetColoredComboBox(
        ui->gpu_accuracy, ui->label_gpu_accuracy,
        static_cast<int>(Settings::values.gpu_accuracy.GetValue(true)));
    ConfigurationShared::SetColoredComboBox(
        ui->anisotropic_filtering_combobox, ui->af_label,
        static_cast<int>(Settings::values.max_anisotropy.GetValue(true)));
}
