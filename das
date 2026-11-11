[33mcommit d0241c604599e7a513ab76ca124edaad17b1b342[m[33m ([m[1;36mHEAD[m[33m -> [m[1;32mmain[m[33m, [m[1;31morigin/main[m[33m, [m[1;31morigin/HEAD[m[33m)[m
Author: Kiarosaurus <kiara.balcazar@utec.edu.pe>
Date:   Tue Nov 10 20:32:18 2026 -0500

    changes when designing levels

 src/app/CustomLevelEditorScene.cpp | 189 [32m++++++++++++++++++++++++++++++++++++++++++[m[31m--------[m
 src/app/CustomLevelEditorScene.hpp |   2 [32m+[m[31m-[m
 src/app/game/MainGameScene.cpp     |   6 [32m+[m[31m-[m
 src/rendering/GameUiRenderer.cpp   |  19 [32m+++++[m
 4 files changed, 184 insertions(+), 32 deletions(-)

[33mcommit 5aef7219cb40aebd6208124792eb9ab02eb3dd99[m
Author: Kiarosaurus <kiara.balcazar@utec.edu.pe>
Date:   Tue Nov 10 00:15:34 2026 -0500

    add level carousel + fix double audio files

 assets/audio/level_1.wav   | Bin [31m4633694[m -> [32m0[m bytes
 assets/audio/level_2.wav   | Bin [31m4656442[m -> [32m0[m bytes
 assets/audio/level_3.wav   | Bin [31m5160526[m -> [32m0[m bytes
 assets/levels/level_4.json | 128 [32m++++++++++++++++++++++++++++++++++++++++++++++++++++++++++[m
 src/app/AppContext.hpp     |   1 [32m+[m
 src/app/LevelCatalog.hpp   |   7 [32m++[m[31m--[m
 src/app/MainMenuScene.cpp  |  70 [32m+++++++++++++++++++++++[m[31m---------[m
 src/app/SceneManager.cpp   |   5 [32m+++[m
 8 files changed, 189 insertions(+), 22 deletions(-)

[33mcommit 74262adb68ade5b7192ae08f78bfdd4d8dd68fa9[m
Author: Kiarosaurus <kiara.balcazar@utec.edu.pe>
Date:   Mon Nov 9 23:43:50 2026 -0500

    add custom music

 assets/audio/custom/A Pedir Su Mano.ogg                     | Bin [31m0[m -> [32m1178479[m bytes
 assets/audio/custom/Abusadora.ogg                           | Bin [31m0[m -> [32m1034150[m bytes
 "assets/audio/custom/Adi\303\263s Pueblo de Ayacucho.ogg"   | Bin [31m0[m -> [32m910456[m bytes
 assets/audio/custom/Aquel Lugar.ogg                         | Bin [31m0[m -> [32m936944[m bytes
 assets/audio/custom/Bombon Asesino.ogg                      | Bin [31m0[m -> [32m850059[m bytes
 assets/audio/custom/CHOP SUEY.ogg                           | Bin [31m0[m -> [32m1328928[m bytes
 assets/audio/custom/Cervecero.ogg                           | Bin [31m0[m -> [32m1379251[m bytes
 assets/audio/custom/Crazy Frog.ogg                          | Bin [31m0[m -> [32m683358[m bytes
 assets/audio/custom/DONT USE THIS SONG I BEG YOU.ogg        | Bin [31m0[m -> [32m872761[m bytes
 assets/audio/custom/Dile.ogg                                | Bin [31m0[m -> [32m836374[m bytes
 assets/audio/custom/El Embrujo.ogg                          | Bin [31m0[m -> [32m975356[m bytes
 assets/audio/custom/El Gran Varon.ogg                       | Bin [31m0[m -> [32m1678948[m bytes
 assets/audio/custom/Elsa.ogg                                | Bin [31m0[m -> [32m897044[m bytes
 "assets/audio/custom/En Tus Tierras Bailar\303\251.ogg"     | Bin [31m0[m -> [32m1078416[m bytes
 assets/audio/custom/Flor de Retama.ogg                      | Bin [31m0[m -> [32m1538643[m bytes
 assets/audio/custom/Gasolina.ogg                            | Bin [31m0[m -> [32m774049[m bytes
 assets/audio/custom/Kulikitaka.ogg                          | Bin [31m0[m -> [32m1070026[m bytes
 assets/audio/custom/La Danza del Petrolero.ogg              | Bin [31m0[m -> [32m927596[m bytes
 "assets/audio/custom/La Due\303\261a del Swing.ogg"         | Bin [31m0[m -> [32m651600[m bytes
 assets/audio/custom/La danza de los Mirlos.ogg              | Bin [31m0[m -> [32m730712[m bytes
 assets/audio/custom/Llama de amor.ogg                       | Bin [31m0[m -> [32m998090[m bytes
 assets/audio/custom/Lloraras.ogg                            | Bin [31m0[m -> [32m908631[m bytes
 "assets/audio/custom/Lo Que Pas\303\263, Pas\303\263.ogg"   | Bin [31m0[m -> [32m857535[m bytes
 assets/audio/custom/Los Cantantes - El Venao.ogg            | Bin [31m0[m -> [32m1171256[m bytes
 assets/audio/custom/Matarina.ogg                            | Bin [31m0[m -> [32m730201[m bytes
 assets/audio/custom/Mayor Que Yo.ogg                        | Bin [31m0[m -> [32m906682[m bytes
 "assets/audio/custom/Me Emborrachar\303\251.ogg"            | Bin [31m0[m -> [32m862176[m bytes
 "assets/audio/custom/Men\303\272 - Cari\303\261ito.ogg"     | Bin [31m0[m -> [32m1019003[m bytes
 assets/audio/custom/N1 - La Colegiala.ogg                   | Bin [31m0[m -> [32m834574[m bytes
 assets/audio/custom/N2 - Soy Provinciano.ogg                | Bin [31m0[m -> [32m872563[m bytes
 assets/audio/custom/N3 - El Aguajal.ogg                     | Bin [31m0[m -> [32m918450[m bytes
 assets/audio/custom/N4 - A Patricia.ogg                     | Bin [31m0[m -> [32m780912[m bytes
 assets/audio/custom/N5 - Motor y Motivo.ogg                 | Bin [31m0[m -> [32m796813[m bytes
 assets/audio/custom/No Te Creas Tan Importante.ogg          | Bin [31m0[m -> [32m1263304[m bytes
 assets/audio/custom/Nunca Pense Llorar.ogg                  | Bin [31m0[m -> [32m617997[m bytes
 assets/audio/custom/Ojos Azules.ogg                         | Bin [31m0[m -> [32m1011852[m bytes
 assets/audio/custom/Pedro Navaja.ogg                        | Bin [31m0[m -> [32m1796046[m bytes
 assets/audio/custom/Pobre Diabla.ogg                        | Bin [31m0[m -> [32m1015691[m bytes
 .../custom/Por qu\303\251 ser\303\241s as\303\255.ogg"      | Bin [31m0[m -> [32m914648[m bytes
 assets/audio/custom/Que Levante la Mano.ogg                 | Bin [31m0[m -> [32m505344[m bytes
 assets/audio/custom/Quimbara.ogg                            | Bin [31m0[m -> [32m1191996[m bytes
 assets/audio/custom/Rakata.ogg                              | Bin [31m0[m -> [32m702473[m bytes
 assets/audio/custom/Suavemente.ogg                          | Bin [31m0[m -> [32m1129145[m bytes
 assets/audio/custom/Tu Retrato.ogg                          | Bin [31m0[m -> [32m549901[m bytes
 assets/audio/custom/Valicha.ogg                             | Bin [31m0[m -> [32m834693[m bytes
 assets/audio/custom/Vivir Mi Vida.ogg                       | Bin [31m0[m -> [32m1044121[m bytes
 src/app/AppContext.hpp                                      |   1 [32m+[m
 src/app/AudioController.cpp                                 |  33 [32m++++++[m[31m-[m
 src/app/AudioController.hpp                                 |   5 [32m++[m
 src/app/CustomLevelCatalog.cpp                              |   5 [32m++[m
 src/app/CustomLevelData.hpp                                 |   3 [32m+[m
 src/app/CustomLevelEditorScene.cpp                          | 120 [32m++++++++++++++++++++++++[m[31m-[m
 src/app/CustomLevelEditorScene.hpp                          |   4 [32m+[m
 src/app/CustomMusicCatalog.hpp                              |  35 [32m++++++++[m
 src/app/MainMenuScene.cpp                                   |  11 [32m++[m[31m-[m
 55 files changed, 210 insertions(+), 7 deletions(-)

[33mcommit 4d94921f3f0fe4dd75608b15c868d3d3985e9136[m
Author: Kiarosaurus <kiara.balcazar@utec.edu.pe>
Date:   Mon Nov 9 01:35:58 2026 -0500

    add custom level design scene

 src/app/App.cpp                    |   9 [32m+[m[31m-[m
 src/app/AppContext.hpp             |   6 [32m++[m
 src/app/CustomLevelCatalog.cpp     |  52 [32m++++++++++[m
 src/app/CustomLevelCatalog.hpp     |  22 [32m++++[m
 src/app/CustomLevelData.hpp        |  16 [32m+++[m
 src/app/CustomLevelEditorScene.cpp | 275 [32m++++++++++++++++++++++++++++++++++++++++++++++++++[m
 src/app/CustomLevelEditorScene.hpp |  20 [32m++++[m
 src/app/MainMenuScene.cpp          | 109 [32m+++++++[m[31m-------------[m
 src/app/MainMenuScene.hpp          |   3 [32m+[m[31m-[m
 src/app/SceneManager.cpp           |   3 [32m+[m[31m-[m
 src/app/SceneManager.hpp           |   1 [32m+[m
 src/app/game/MainGameScene.cpp     |  39 [32m++++++[m[31m-[m
 src/app/game/MainGameScene.hpp     |   1 [32m+[m
 src/app/game/PlaySession.cpp       |   6 [32m+[m[31m-[m
 src/physics/Box2D_Physics.cpp      |  31 [32m++++++[m
 src/physics/Box2