#ifndef APP_CUSTOM_MUSIC_CATALOG_HPP
#define APP_CUSTOM_MUSIC_CATALOG_HPP

#include <string>
#include <vector>

struct CustomMusicTrack {
    std::string title;
    std::string artist;
    std::string path;
};

struct CustomMusicCategory {
    std::string name;
    std::vector<CustomMusicTrack> tracks;
};

inline const std::vector<CustomMusicCategory>& GetCustomMusicCategories() {
    static const std::vector<CustomMusicCategory> categories{
        {"Chicha", {{"Soy Provinciano", "Chacalón", "assets/audio/custom/N2 - Soy Provinciano.ogg"}, {"El Aguajal", "Los Shapis", "assets/audio/custom/N3 - El Aguajal.ogg"}, {"A Patricia", "Los Destellos", "assets/audio/custom/N4 - A Patricia.ogg"}, {"Elsa", "Los Destellos", "assets/audio/custom/Elsa.ogg"}, {"La Danza de los Mirlos", "Los Mirlos", "assets/audio/custom/La danza de los Mirlos.ogg"}}},
        {"Amazónica", {{"La Danza de los Mirlos", "Los Mirlos", "assets/audio/custom/La danza de los Mirlos.ogg"}, {"La Danza del Petrolero", "Los Mirlos", "assets/audio/custom/La Danza del Petrolero.ogg"}, {"El Aguajal", "Los Shapis", "assets/audio/custom/N3 - El Aguajal.ogg"}, {"A Patricia", "Los Destellos", "assets/audio/custom/N4 - A Patricia.ogg"}}},
        {"Psicodélica", {{"A Patricia", "Los Destellos", "assets/audio/custom/N4 - A Patricia.ogg"}, {"Elsa", "Los Destellos", "assets/audio/custom/Elsa.ogg"}, {"La Danza de los Mirlos", "Los Mirlos", "assets/audio/custom/La danza de los Mirlos.ogg"}, {"La Danza del Petrolero", "Los Mirlos", "assets/audio/custom/La Danza del Petrolero.ogg"}}},
        {"Cumbia", {{"Cariñito", "Los Hijos del Sol", "assets/audio/custom/Menú - Cariñito.ogg"}, {"La Colegiala", "Rodolfo y su Típica RA7", "assets/audio/custom/N1 - La Colegiala.ogg"}, {"Motor y Motivo", "Grupo 5", "assets/audio/custom/N5 - Motor y Motivo.ogg"}, {"El Embrujo", "Grupo 5", "assets/audio/custom/El Embrujo.ogg"}, {"El Cervecero", "Armonía 10", "assets/audio/custom/Cervecero.ogg"}, {"Llama de Amor", "Agua Marina", "assets/audio/custom/Llama de amor.ogg"}}},
        {"Norteña", {{"Motor y Motivo", "Grupo 5", "assets/audio/custom/N5 - Motor y Motivo.ogg"}, {"El Embrujo", "Grupo 5", "assets/audio/custom/El Embrujo.ogg"}, {"El Cervecero", "Armonía 10", "assets/audio/custom/Cervecero.ogg"}, {"Llama de Amor", "Agua Marina", "assets/audio/custom/Llama de amor.ogg"}, {"Que Levante la Mano", "Hermanos Yaipén", "assets/audio/custom/Que Levante la Mano.ogg"}}},
        {"Tecnocumbia", {{"Nunca Pensé Llorar", "Rossy War", "assets/audio/custom/Nunca Pense Llorar.ogg"}, {"No Te Creas Tan Importante", "Deyvis Orosco", "assets/audio/custom/No Te Creas Tan Importante.ogg"}, {"Me Emborracharé", "Papillón", "assets/audio/custom/Me Emborracharé.ogg"}, {"Bombón Asesino", "Alma Bella", "assets/audio/custom/Bombon Asesino.ogg"}, {"Que Levante la Mano", "Hermanos Yaipén", "assets/audio/custom/Que Levante la Mano.ogg"}, {"¿Por Qué Serás Así?", "Clavito & Su Chela", "assets/audio/custom/Por qué serás así.ogg"}}},
        {"Reguetón", {{"Gasolina", "Daddy Yankee", "assets/audio/custom/Gasolina.ogg"}, {"Lo Que Pasó, Pasó", "Daddy Yankee", "assets/audio/custom/Lo Que Pasó, Pasó.ogg"}, {"Dile", "Don Omar", "assets/audio/custom/Dile.ogg"}, {"Pobre Diabla", "Don Omar", "assets/audio/custom/Pobre Diabla.ogg"}, {"Rakata", "Wisin & Yandel", "assets/audio/custom/Rakata.ogg"}, {"Mayor Que Yo", "Wisin & Yandel", "assets/audio/custom/Mayor Que Yo.ogg"}}},
        {"Salsa", {{"Llorarás", "Oscar D’León", "assets/audio/custom/Lloraras.ogg"}, {"Quimbara", "Celia Cruz", "assets/audio/custom/Quimbara.ogg"}, {"Vivir Mi Vida", "Marc Anthony", "assets/audio/custom/Vivir Mi Vida.ogg"}, {"El Gran Varón", "Willie Colón", "assets/audio/custom/El Gran Varon.ogg"}, {"Pedro Navaja", "Rubén Blades", "assets/audio/custom/Pedro Navaja.ogg"}, {"Aquel Lugar", "Frankie Ruiz", "assets/audio/custom/Aquel Lugar.ogg"}}},
        {"Merengue", {{"Suavemente", "Elvis Crespo", "assets/audio/custom/Suavemente.ogg"}, {"La Dueña del Swing", "Los Hermanos Rosario", "assets/audio/custom/La Dueña del Swing.ogg"}, {"El Venao", "Los Cantantes", "assets/audio/custom/Los Cantantes - El Venao.ogg"}, {"Kulikitaka", "Toño Rosario", "assets/audio/custom/Kulikitaka.ogg"}, {"A Pedir Su Mano", "Juan Luis Guerra", "assets/audio/custom/A Pedir Su Mano.ogg"}, {"Abusadora", "Wilfrido Vargas", "assets/audio/custom/Abusadora.ogg"}}},
        {"Huayno", {{"Ojos Azules", "Autor anónimo", "assets/audio/custom/Ojos Azules.ogg"}, {"Matarina", "Autor anónimo", "assets/audio/custom/Matarina.ogg"}, {"Valicha", "Miguel Ángel Hurtado Delgado", "assets/audio/custom/Valicha.ogg"}, {"Adiós Pueblo de Ayacucho", "Autor anónimo", "assets/audio/custom/Adiós Pueblo de Ayacucho.ogg"}, {"Flor de Retama", "Ricardo Dolorier Urbano", "assets/audio/custom/Flor de Retama.ogg"}, {"Tu Retrato", "", "assets/audio/custom/Tu Retrato.ogg"}}},
        {"???", {{"CHOP SUEY", "", "assets/audio/custom/CHOP SUEY.ogg"}, {"Crazy Frog", "", "assets/audio/custom/Crazy Frog.ogg"}, {"DONT USE THIS SONG I BEG YOU", "", "assets/audio/custom/DONT USE THIS SONG I BEG YOU.ogg"}, {"En Tus Tierras Bailaré", "", "assets/audio/custom/En Tus Tierras Bailaré.ogg"}}}
    };
    return categories;
}

#endif
