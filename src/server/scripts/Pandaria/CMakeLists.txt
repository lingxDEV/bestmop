# Copyright (C) 2008-2021 ZonaXtrema group (TheDark)
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

file(GLOB_RECURSE scripts_GateSettingSun Pandaria/GateSettingSun/*.cpp Pandaria/GateSettingSun/*.h)
file(GLOB_RECURSE scripts_HeartOfFear Pandaria/HeartOfFear/*.cpp Pandaria/HeartOfFear/*.h)
file(GLOB_RECURSE scripts_MogushanPalace Pandaria/MogushanPalace/*.cpp Pandaria/MogushanPalace/*.h)
file(GLOB_RECURSE scripts_MogushanVaults Pandaria/MogushanVaults/*.cpp Pandaria/MogushanVaults/*.h)
file(GLOB_RECURSE scripts_ShadopanMonastery Pandaria/ShadopanMonastery/*.cpp Pandaria/ShadopanMonastery/*.h)
file(GLOB_RECURSE scripts_SiegeOfNiuzaoTemple Pandaria/SiegeOfNiuzaoTemple/*.cpp Pandaria/SiegeOfNiuzaoTemple/*.h)
file(GLOB_RECURSE scripts_SiegeOfOrgrimmar Pandaria/SiegeOfOrgrimmar/*.cpp Pandaria/SiegeOfOrgrimmar/*.h)
file(GLOB_RECURSE scripts_StormstoutBrewery Pandaria/StormstoutBrewery/*.cpp Pandaria/StormstoutBrewery/*.h)
file(GLOB_RECURSE scripts_TempleJadeSerpent Pandaria/TempleJadeSerpent/*.cpp Pandaria/TempleJadeSerpent/*.h)
file(GLOB_RECURSE scripts_TerraceOfEndlessSpring Pandaria/TerraceOfEndlessSpring/*.cpp Pandaria/TerraceOfEndlessSpring/*.h)
file(GLOB_RECURSE scripts_ThroneOfThunder Pandaria/ThroneOfThunder/*.cpp Pandaria/ThroneOfThunder/*.h)
file(GLOB_RECURSE scripts_WanderingIsland Pandaria/WanderingIsland/*.cpp Pandaria/WanderingIsland/*.h)
file(GLOB_RECURSE scripts_world_bosses Pandaria/WorldBosses/*.cpp Pandaria/WorldBosses/*.h Pandaria/WorldBosses/AugustCelestials/*.cpp Pandaria/WorldBosses/AugustCelestials/*.h)
file(GLOB scripts_PandariaCustom Pandaria/*.cpp Pandaria/*.h)
#file(GLOB_RECURSE scripts_Timelessisle Pandaria/timelessisle/*.cpp Pandaria/timelessisle/*.h)

source_group(Pandaria\\GateSettingSun FILES ${scripts_GateSettingSun})
source_group(Pandaria\\HeartOfFear FILES ${scripts_HeartOfFear})
source_group(Pandaria\\MogushanPalace FILES ${scripts_MogushanPalace})
source_group(Pandaria\\MogushanVaults FILES ${scripts_MogushanVaults})
source_group(Pandaria\\ShadopanMonastery FILES ${scripts_ShadopanMonastery})
source_group(Pandaria\\SiegeOfNiuzaoTemple FILES ${scripts_SiegeOfNiuzaoTemple})
source_group(Pandaria\\SiegeOfOrgrimmar FILES ${scripts_SiegeOfOrgrimmar})
source_group(Pandaria\\StormstoutBrewery FILES ${scripts_StormstoutBrewery})
source_group(Pandaria\\TempleJadeSerpent FILES ${scripts_TempleJadeSerpent})
source_group(Pandaria\\TerraceOfEndlessSpring FILES ${scripts_TerraceOfEndlessSpring})
source_group(Pandaria\\ThroneOfThunder FILES ${scripts_ThroneOfThunder})
source_group(Pandaria\\WanderingIsland FILES ${scripts_WanderingIsland})
source_group(Pandaria\\world_bosses FILES ${scripts_world_bosses})
source_group(Pandaria\\Custom FILES ${scripts_PandariaCustom})

#source_group(Pandaria\\TimelessIsle FILES ${scripts_Timelessisle})

set(scripts_STAT_SRCS
  ${scripts_STAT_SRCS}
  ${scripts_GateSettingSun}
  ${scripts_HeartOfFear}
  ${scripts_MogushanPalace}
  ${scripts_MogushanVaults}
  ${scripts_ShadopanMonastery}
  ${scripts_SiegeOfNiuzaoTemple}
  ${scripts_SiegeOfOrgrimmar}
  ${scripts_StormstoutBrewery}
  ${scripts_TempleJadeSerpent}
  ${scripts_TerraceOfEndlessSpring}
  ${scripts_ThroneOfThunder}
  ${scripts_WanderingIsland}
  #${scripts_Timelessisle}
  ${scripts_world_bosses}
  ${scripts_PandariaCustom}
)

message("  -> Prepared: Pandaria")
