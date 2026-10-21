// SPDX-License-Identifier: GPL-3.0-only

#include <utility>
#include <vector>

#include "../../halo_data/particle.hpp"

#include "particle.hpp"

namespace Chimera {
    struct InterpolatedParticle {
        bool interpolate;
        Point3D position;
    };

    // These swap every tick. They grow to fit the particle table, which mods can extend past 1024.
    static std::vector<InterpolatedParticle> current_tick;
    static std::vector<InterpolatedParticle> previous_tick;

    // If true, a tick has passed and it's time to re-copy the particle data.
    static bool tick_passed = false;

    void interpolate_particle() noexcept {
        auto &particle_table = ParticleTable::get_particle_table();
        if(tick_passed) {
            // Swap buffers.
            std::swap(current_tick, previous_tick);
            if(current_tick.size() < particle_table.max_elements) {
                current_tick.resize(particle_table.max_elements);
                previous_tick.resize(particle_table.max_elements);
            }

            // Go through each particle, determining if any can be interpolated.
            for(std::size_t i = 0, buffer_size = current_tick.size(); i < buffer_size; i++) {
                auto *particle = particle_table.get_element(i);
                auto &current_tick_particle = current_tick[i];
                current_tick_particle.interpolate = false;

                if(!particle) {
                    continue;
                }

                // Copy the original particle data
                current_tick_particle.position = particle->position;

                // I'm not entirely sure what unknown0 does, but it magically determines if I should interpolate the particle.
                current_tick_particle.interpolate = particle->unknown0 & 0xFFFF;
            }

            tick_passed = false;
        }

        // Iterate through each particle
        for(std::size_t i = 0, buffer_size = current_tick.size(); i < particle_table.current_size && i < buffer_size; i++) {
            auto *particle = particle_table.first_element + i;
            auto &current_tick_particle = current_tick[i];
            auto &previous_tick_particle = previous_tick[i];
            extern float interpolation_tick_progress;

            // Interpolate each particle that can be interpolated.
            if(current_tick_particle.interpolate && previous_tick_particle.interpolate) {
                interpolate_point(previous_tick_particle.position, current_tick_particle.position, particle->position, interpolation_tick_progress);
            }
        }
    }

    void interpolate_particle_after() noexcept {
        auto &particle_table = ParticleTable::get_particle_table();
        for(std::size_t i = 0, buffer_size = current_tick.size(); i < particle_table.current_size && i < buffer_size; i++) {
            auto *particle = particle_table.get_element(i);
            auto &current_tick_particle = current_tick[i];
            auto &previous_tick_particle = previous_tick[i];

            // Restore each position.
            if(current_tick_particle.interpolate && previous_tick_particle.interpolate) {
                particle->position = current_tick_particle.position;
            }
        }
    }

    void interpolate_particle_clear() noexcept {
        for(std::size_t i = 0, buffer_size = current_tick.size(); i < buffer_size; i++) {
            current_tick[i].interpolate = false;
            previous_tick[i].interpolate = false;
        }
    }

    void interpolate_particle_on_tick() noexcept {
        tick_passed = true;
    }
}
