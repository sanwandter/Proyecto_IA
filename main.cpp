#include <bits/stdc++.h>
using namespace std;

const int MAX_ITER = 10000;
const int TABU_SIZE = 15;
const int CANDIDATE_LIST_SIZE = 200;  // Tamaño de la lista de candidatos

struct Cell {
    int id;
    string site;
    int demand;
    set<int> lbc; // Frecuencias bloqueadas localmente
    int tokens_leidos = 0;
};

struct Interference {
    double v_co = 0.0;
    double v_adj = 0.0;
};

class Problem {
public:
    int fmin, fmax;
    int co_site_sep;
    set<int> globally_blocked;
    map<int, Cell> cells;
    map<pair<int, int>, Interference> relations;
    map<int, vector<int>> domains;
    vector<int> freqs_disponibles;

    bool load(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: No se pudo abrir el archivo " << filename << endl;
            return false;
        }

        string line, sec;
        int curr_cell = -1;
        pair<int, int> curr_rel;

        while (getline(file, line)) {
            size_t pos = line.find('#');
            if (pos != string::npos) line = line.substr(0, pos);
            
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (line.empty()) continue;

            if (line.find("GENERAL_INFORMATION") != string::npos) sec = "GEN";
            else if (line.find("CELL_RELATIONS") != string::npos) sec = "RELS";
            else if (line.find("CELLS") != string::npos) sec = "CELLS";
            else if (sec == "GEN") parse_gen(line);
            else if (sec == "CELLS") parse_cells(line, curr_cell);
            else if (sec == "RELS") parse_rels(line, curr_rel);
            
            // Resetear contexto DESPUES de procesar la linea
            if (line.find("}") != string::npos && line.find("{") == string::npos) {
                curr_cell = -1;
                curr_rel = {-1, -1};
            }
        }
        calc_domains();
        return true;
    }

private:
    void parse_gen(const string& line) {
        stringstream ss(line);
        string key;
        ss >> key;
        if (key == "SPECTRUM") {
            char c;
            ss >> c >> fmin >> c >> fmax >> c;
        } else if (key == "CO_SITE_SEPARATION") {
            ss >> co_site_sep; 
        } else if (key == "GLOBALLY_BLOCKED_CHANNELS") {
            int f;
            while (ss >> f) globally_blocked.insert(f);
        }
    }

    void parse_cells(const string& line, int& cid) {
        stringstream ss(line);
        if (cid == -1) {
            int id;
            char c;
            ss >> id >> c;
            if (c == '{') {
                cid = id;
                cells[cid].id = id;
                cells[cid].demand = 0;
                cells[cid].tokens_leidos = 0;
            }
        } else {
            string tok;
            ss >> tok;
            if (tok.empty()) return;
            
            if (tok == "LBC") {
                int f;
                while (ss >> f) cells[cid].lbc.insert(f);
            } else if (tok == "LOC") {
                
            } else if (tok.find('}') != string::npos) {
                cid = -1;
            } else {
                vector<string> clean_toks;
                
                string ct = tok;
                if (!ct.empty() && ct.back() == ';') ct.pop_back();
                if (!ct.empty()) clean_toks.push_back(ct);
                
                while (ss >> tok && tok != "LBC" && tok != "LOC" && tok.find('}') == string::npos) {
                    ct = tok;
                    if (!ct.empty() && ct.back() == ';') ct.pop_back();
                    if (!ct.empty()) clean_toks.push_back(ct);
                }
                
                for (const string& t : clean_toks) {
                    if (cells[cid].tokens_leidos == 0) {
                        cells[cid].site = t;
                        cells[cid].tokens_leidos++;
                    } else if (cells[cid].tokens_leidos == 1) {
                        cells[cid].tokens_leidos++;
                    } else if (cells[cid].tokens_leidos == 2) {
                        cells[cid].demand = stoi(t);
                        cells[cid].tokens_leidos++;
                        break;
                    }
                }
            }
        }
    }

    void parse_rels(const string& line, pair<int, int>& cp) {
        stringstream ss(line);
        int c1, c2;
        char c;
        if (ss >> c1 >> c2 >> c) {
            if (c == '{') {
                cp = {c1, c2};
            }
        }
        
        if (cp.first != -1) {
            stringstream ss2(line);
            string token;
            while (ss2 >> token) {
                if (token == "DA") {
                    double v1 = 0.0, v2 = 0.0;
                    ss2 >> v1;
                    string next;
                    if (ss2 >> next) {
                        // Limpiar el ';' si existe
                        if (!next.empty() && next.back() == ';') {
                            next.pop_back();
                        }
                        // Intentar convertir
                        try {
                            if (!next.empty() && next.find_first_not_of("0123456789.-") == string::npos) {
                                v2 = stod(next);
                            }
                        } catch (...) {
                            v2 = 0.0;
                        }
                    }
                    relations[cp].v_co = v1;
                    relations[cp].v_adj = v2;
                    break;
                }
                if (token.find('}') != string::npos) {
                    cp = {-1, -1};
                    break;
                }
            }
        }
    }

    void calc_domains() {
        // armar lista de frecuencias disponibles
        for (int f = fmin; f <= fmax; ++f) {
            if (globally_blocked.find(f) == globally_blocked.end()) {
                freqs_disponibles.push_back(f);
            }
        }

        // dominio por celda = freqs disponibles - LBC
        for (auto& [id, cell] : cells) {
            for (int f : freqs_disponibles) {
                if (cell.lbc.find(f) == cell.lbc.end()) {
                    domains[id].push_back(f);
                }
            }
        }
    }
};


class Solution {
public:
    map<int, vector<int>> asignacion;
    Problem* prob;
    double costo;

    Solution(Problem* p) : prob(p), costo(1e9) {}
    Solution() : prob(nullptr), costo(1e9) {}

    bool es_factible() {
        for (auto& [id1, c1] : prob->cells) {
            auto& f1 = asignacion[id1];
            
            for (size_t i = 0; i < f1.size(); ++i) {
                for (size_t j = i + 1; j < f1.size(); ++j) {
                    if (abs(f1[i] - f1[j]) < prob->co_site_sep) return false;
                }
            }
            
            for (auto& [id2, c2] : prob->cells) {
                if (id1 >= id2) continue;
                if (c1.site != c2.site) continue;
                
                auto& f2 = asignacion[id2];
                for (int fi : f1) {
                    for (int fj : f2) {
                        if (abs(fi - fj) < prob->co_site_sep) return false;
                    }
                }
            }
        }
        return true;
    }
    
    bool puede_agregar(int cell_id, int freq) {
        for (int f : asignacion[cell_id]) {
            if (abs(f - freq) < prob->co_site_sep) return false;
        }
        
        string sitio = prob->cells[cell_id].site;
        for (auto& [oid, ocell] : prob->cells) {
            if (oid == cell_id) continue;
            if (ocell.site != sitio) continue;
            
            for (int f : asignacion[oid]) {
                if (abs(f - freq) < prob->co_site_sep) return false;
            }
        }
        return true;
    }

    void generar_inicial() {
        random_device rd;
        mt19937 gen(rd());
        // Garantizar asignacion inicial factible segun restricciones duras
        for (auto& [id, cell] : prob->cells) {
            auto& dom = prob->domains[id];
            if (dom.size() < (size_t)cell.demand) {
                cerr << "ERROR: Dominio insuficiente para celda " << id << endl;
                exit(1);
            }
        }
        
        for (auto& [id, cell] : prob->cells) {
            asignacion[id].clear();
            auto& dom = prob->domains[id];
            
            for (int k = 0; k < cell.demand; ++k) {
                vector<int> factibles;
                for (int f : dom) {
                    if (puede_agregar(id, f)) factibles.push_back(f);
                }
                
                if (factibles.empty()) {
                    factibles = dom;
                    if (factibles.empty()) {
                        cerr << "ERROR: Dominio vacio para celda " << id << endl;
                        exit(1);
                    }
                }
                
                uniform_int_distribution<> dis(0, factibles.size() - 1);
                asignacion[id].push_back(factibles[dis(gen)]);
            }
        }
        
        calcular_costo();
    }

    void calcular_costo() {
        costo = 0.0;
        for (auto& [par, inf] : prob->relations) {
            int i = par.first;
            int j = par.second;
            if (asignacion.find(i) == asignacion.end() || 
                asignacion.find(j) == asignacion.end()) continue;

            for (int fi : asignacion[i]) {
                for (int fj : asignacion[j]) {
                    if (fi == fj) {
                        costo += inf.v_co;
                    } else if (abs(fi - fj) == 1) {
                        costo += inf.v_adj;
                    }
                }
            }
        }
    }
};


class TabuSearch {
public:
    Problem problema;
    Solution mejor;
    map<tuple<int, int, int>, int> lista_tabu;

    void ejecutar(const string& archivo) {
        if (!problema.load(archivo)) return;

        string log_file = archivo.substr(0, archivo.find('.')) + "_log.txt";
        ofstream log(log_file);
        
        cout << "Instancia: " << archivo << endl;
        cout << "Celdas: " << problema.cells.size() << endl;
        
        log << "Instancia: " << archivo << endl;
        log << "Celdas: " << problema.cells.size() << endl;
        
        int total_trxs = 0;
        for (auto& [id, c] : problema.cells) total_trxs += c.demand;
        cout << "TRXs: " << total_trxs << endl;
        cout << "Relaciones DA: " << problema.relations.size() << endl;
        log << "TRXs: " << total_trxs << endl;
        log << "Relaciones DA: " << problema.relations.size() << endl;
        log << "Espectro: [" << problema.fmin << ", " << problema.fmax << "]" << endl;
        log << "CO_SITE_SEPARATION: " << problema.co_site_sep << endl;
        log << "MAX_ITER: " << MAX_ITER << endl;
        log << "TABU_SIZE: " << TABU_SIZE << endl;
        log << endl;
        
        Solution actual(&problema);
        actual.generar_inicial();
        mejor = actual;

        cout << "Costo inicial: " << mejor.costo << endl;
        log << "Costo inicial: " << mejor.costo << endl << endl;

        for (int it = 0; it < MAX_ITER; ++it) {
            Solution vecino = buscar_vecino_con_candidate_list(actual, it);
            
            if (vecino.prob == nullptr) {
                log << "No hay mas vecinos factibles en iter " << (it + 1) << endl;
                break;
            }

            actual = vecino;

            if (actual.costo < mejor.costo) {
                mejor = actual;
            }
            
            string line = "Iter " + to_string(it + 1) + ": actual = " + 
                         to_string(actual.costo) + ", mejor = " + to_string(mejor.costo);
            cout << line << endl;
            log << line << endl;
            
            // Terminar si se encuentra solución óptima (costo 0)
            if (mejor.costo == 0.0) {
                log << "Solucion optima encontrada (costo = 0) en iter " << (it + 1) << endl;
                cout << "Solucion optima encontrada (costo = 0)" << endl;
                break;
            }
        }

        cout << "\nCosto final: " << mejor.costo << endl;
        log << endl << "Costo final: " << mejor.costo << endl;
        
        if (mejor.es_factible()) {
            cout << "Solucion factible" << endl;
            log << "Solucion factible" << endl;
        } else {
            cout << "Solucion infactible" << endl;
            log << "Solucion infactible" << endl;
        }
        
        log.close();
        cout << "Log guardado en: " << log_file << endl;
        
        // Guardar solucion final
        guardar_solucion(archivo);
    }
    
    void guardar_solucion(const string& archivo) {
        string sol_file = archivo.substr(0, archivo.find('.')) + "_solution.txt";
        ofstream out(sol_file);
        
        out << "=== SOLUCION FAP ===" << endl << endl;
        out << "Instancia: " << archivo << endl;
        out << "Costo total: " << mejor.costo << endl;
        out << "Factible: " << (mejor.es_factible() ? "SI" : "NO") << endl << endl;
        
        out << "Asignacion de frecuencias:" << endl;
        out << "Celda\tTRX\tFrecuencia" << endl;
        out << "--------------------------" << endl;
        
        for (auto& [id, freqs] : mejor.asignacion) {
            for (size_t i = 0; i < freqs.size(); ++i) {
                out << id << "\t" << (i + 1) << "\t" << freqs[i] << endl;
            }
        }
        
        out.close();
        cout << "Solucion guardada en: " << sol_file << endl;
    }

    bool es_factible_trx(int cell_id, size_t trx_idx, int freq, const map<int, vector<int>>& asig) {
        for (size_t i = 0; i < asig.at(cell_id).size(); ++i) {
            if (i == trx_idx) continue;
            if (abs(asig.at(cell_id)[i] - freq) < problema.co_site_sep) return false;
        }
        
        string sitio = problema.cells[cell_id].site;
        for (auto& [oid, ocell] : problema.cells) {
            if (oid == cell_id) continue;
            if (ocell.site != sitio) continue;
            
            for (int f : asig.at(oid)) {
                if (abs(f - freq) < problema.co_site_sep) return false;
            }
        }
        return true;
    }

private:
    // Generar movimientos candidatos aleatorios
    vector<tuple<int, size_t, int, int>> generar_candidatos(const Solution& s, int num_candidatos) {
        random_device rd;
        mt19937 gen(rd());
        
        vector<tuple<int, size_t, int, int>> candidatos;  // (cell_id, trx_idx, old_f, new_f)
        
        // Crear lista de todas las celdas
        vector<int> cell_ids;
        for (auto& [cid, freqs] : s.asignacion) {
            cell_ids.push_back(cid);
        }
        
        // Generar candidatos aleatorios
        uniform_int_distribution<> cell_dist(0, cell_ids.size() - 1);
        
        int intentos = 0;
        int max_intentos = num_candidatos * 5;  // Intentar hasta 5x para encontrar factibles
        
        while (candidatos.size() < (size_t)num_candidatos && intentos < max_intentos) {
            intentos++;
            
            // Seleccionar celda aleatoria
            int cid = cell_ids[cell_dist(gen)];
            
            // Seleccionar TRX aleatorio de esa celda
            if (s.asignacion.at(cid).empty()) continue;
            uniform_int_distribution<> trx_dist(0, s.asignacion.at(cid).size() - 1);
            size_t trx_idx = trx_dist(gen);
            
            // Seleccionar frecuencia aleatoria del dominio
            auto& domain = problema.domains[cid];
            if (domain.empty()) continue;
            uniform_int_distribution<> freq_dist(0, domain.size() - 1);
            int new_f = domain[freq_dist(gen)];
            
            int old_f = s.asignacion.at(cid)[trx_idx];
            
            // Validar movimiento
            if (new_f == old_f) continue;
            if (!es_factible_trx(cid, trx_idx, new_f, s.asignacion)) continue;
            
            // Evitar duplicados
            auto mov = make_tuple(cid, trx_idx, old_f, new_f);
            if (find(candidatos.begin(), candidatos.end(), mov) != candidatos.end()) continue;
            
            candidatos.push_back(mov);
        }
        
        return candidatos;
    }
    
    Solution buscar_vecino_con_candidate_list(const Solution& s, int iter) {
        random_device rd;
        mt19937 gen(rd());
        
        // FASE 1: Generar lista de candidatos
        auto candidatos = generar_candidatos(s, CANDIDATE_LIST_SIZE);
        
        if (candidatos.empty()) {
            Solution vacia;
            return vacia;
        }
        
        // FASE 2: Evaluar todos los candidatos y elegir el mejor
        Solution mejor_vecino;
        double mejor_costo = 1e9;
        tuple<int, size_t, int> mejor_mov_inverso;
        bool encontrado = false;
        
        for (auto& [cid, trx_idx, old_f, new_f] : candidatos) {
            // Verificar si está tabú
            auto tm = make_tuple(cid, trx_idx, new_f);
            bool tabu = lista_tabu.count(tm) && lista_tabu[tm] > iter;
            
            if (tabu) continue;
            
            // Evaluar vecino
            Solution v = s;
            v.asignacion[cid][trx_idx] = new_f;
            v.calcular_costo();
            
            // Guardar el mejor (mejora o empeoramiento)
            if (v.costo < mejor_costo) {
                mejor_vecino = v;
                mejor_costo = v.costo;
                mejor_mov_inverso = make_tuple(cid, trx_idx, old_f);
                encontrado = true;
            }
        }
        
        // FASE 3: Aceptar el mejor candidato
        if (encontrado) {
            lista_tabu[mejor_mov_inverso] = iter + TABU_SIZE;
            return mejor_vecino;
        }
        
        Solution vacia;
        return vacia;
    }
};

int main(int argc, char* argv[]) {
    string archivo = "Tiny.scen";
    if (argc > 1) archivo = argv[1];
    
    TabuSearch ts;
    ts.ejecutar(archivo);
    return 0;
}