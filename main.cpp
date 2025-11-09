#include <bits/stdc++.h>
using namespace std;

const int MAX_ITER = 50000;
const int TABU_SIZE = 15;

struct Cell {
    int id;
    string site;
    int demand;
    set<int> lbc; // Frecuencias bloqueadas localmente
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
            else if (line.find("CELLS") != string::npos) sec = "CELLS";
            else if (line.find("CELL_RELATIONS") != string::npos) sec = "RELS";
            else if (line.find("}") != string::npos) {
                curr_cell = -1;
                curr_rel = {-1, -1};
            }
            else if (sec == "GEN") parse_gen(line);
            else if (sec == "CELLS") parse_cells(line, curr_cell);
            else if (sec == "RELS") parse_rels(line, curr_rel);
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
            }
        } else {
            string tok;
            ss >> tok;
            if (tok.empty()) return;
            
            if (tok == "LBC") {
                int f;
                while (ss >> f) cells[cid].lbc.insert(f);
            } else if (tok == "LOC") {
                // ignorar
            } else {
                vector<string> toks;
                toks.push_back(tok);
                while (ss >> tok) {
                    if (tok == "LBC") {
                        int f;
                        while (ss >> f) cells[cid].lbc.insert(f);
                        break;
                    }
                    toks.push_back(tok);
                }
                
                for (const auto& t : toks) {
                    string ct = t;
                    if (!ct.empty() && ct.back() == ';') ct.pop_back();
                    if (ct.empty()) continue;
                    
                    bool isnum = all_of(ct.begin(), ct.end(), ::isdigit);
                    if (isnum) {
                        cells[cid].demand = stoi(ct);
                    } else {
                        if (cells[cid].site.empty()) cells[cid].site = ct;
                    }
                }
            }
        }
    }

    void parse_rels(const string& line, pair<int, int>& cp) {
        stringstream ss(line);
        if (cp.first == -1) {
            int c1, c2;
            char c;
            ss >> c1 >> c2 >> c;
            if (c == '{') cp = {c1, c2};
        } else {
            string key;
            ss >> key;
            if (key == "DA") { 
                double v1, v2 = 0.0;
                ss >> v1;
                if (ss >> v2) {
                    relations[cp].v_co = v1;
                    relations[cp].v_adj = v2;
                } else {
                    relations[cp].v_co = v1;
                    relations[cp].v_adj = 0.0;
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
            if (domains[id].size() < (size_t)cell.demand) {
                cerr << "Celda " << id << " no tiene suficientes frecuencias" << endl;
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
            asignacion[id].clear();
            auto& dom = prob->domains[id]; // Dominio de la celda valido
            
            // Asignar frecuencias segun demanda
            for (int k = 0; k < cell.demand; ++k) {
                vector<int> factibles; // Frecuencias factibles segun restriccion co-site
                for (int f : dom) {
                    if (puede_agregar(id, f)) factibles.push_back(f);
                }
                
                if (factibles.empty()) {
                    cerr << "No hay frecuencias factibles para celda " << id << endl;
                    continue;
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
        log << "TRXs: " << total_trxs << endl;
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
            Solution vecino = buscar_vecino(actual, it);
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
    Solution buscar_vecino(const Solution& s, int iter) {
        for (auto& [cid, freqs] : s.asignacion) {
            for (size_t idx = 0; idx < freqs.size(); ++idx) {
                int old_f = freqs[idx];
                
                for (int new_f : problema.domains[cid]) {
                    if (new_f == old_f) continue;
                    if (!es_factible_trx(cid, idx, new_f, s.asignacion)) continue;

                    Solution v = s;
                    v.asignacion[cid][idx] = new_f;
                    v.calcular_costo();

                    auto tm = make_tuple(cid, idx, old_f);
                    bool tabu = lista_tabu.count(tm) && lista_tabu[tm] > iter;
                    if (!tabu && v.costo < s.costo) {
                        lista_tabu[tm] = iter + TABU_SIZE;
                        return v;
                    }
                }
            }
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