//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <stdexcept>
#include <string>
#include <vector>

#include "mp_solver.h"

namespace mvrp {
    MPSolver::IloData MPSolver::solve(const ColumnPool &pool, const std::vector<PortWithType>& ports_with_equality, bool linear) const {
        // Calculate the sum of all penalties
        auto all_penalties = 0.0;
        for(const auto &p : prob->data.ports) {
            all_penalties += p->pickup_penalty + p->delivery_penalty;
        }

        IloEnv env;
        IloModel model(env);

        IloNumVarArray var(env);
        IloRangeArray port_constr(env);
        IloRangeArray vc_constr(env);

        IloObjective obj = IloMinimize(env, all_penalties);

        auto np = prob->data.num_ports;
        auto nv = prob->data.num_vessel_classes;

        for(auto i = 1; i <= 2 * (np-1); i++) {
            port_constr.add(IloRange(env, should_row_have_equality(i, ports_with_equality) ? 1.0 : -IloInfinity, 1.0));
        }

        for(auto i = 0; i < nv; i++) {
            vc_constr.add(IloRange(env, -IloInfinity, prob->data.vessel_classes[i]->num_vessels));
        }

        auto col_n = 0;
        for(auto cit = pool.begin(); cit != pool.end(); ++cit) {
            IloNumColumn ilo_c = obj(cit->obj_coeff);

            for(auto i = 1; i < np; i++) {
                ilo_c += port_constr[i - 1](cit->port_coeff[i - 1]); // Pickup port
                ilo_c += port_constr[np - 1 + i - 1](cit->port_coeff[np - 1 + i - 1]); // Delivery port
            }
            for(auto i = 0; i < nv; i++) {
                ilo_c += vc_constr[i](cit->vc_coeff[i]);
            }

            IloNumVar v(ilo_c, 0, IloInfinity, (linear ? IloNumVar::Float : IloNumVar::Bool),
                        ("theta_" + std::to_string(col_n++)).c_str());
            var.add(v);
        }

        model.add(obj);
        model.add(port_constr);
        model.add(vc_constr);

        IloCplex cplex(model);
        cplex.setParam(IloCplex::Threads, prob->params.cplex_cores);
        cplex.setOut(env.getNullStream());

        auto solved = false;

        try {
            solved = cplex.solve();
        } catch(IloException &e) {
            std::cerr << "IloException: " << e << std::endl;
        }

        if(!solved) {
            throw std::runtime_error("Infeasible problem!");
        }

        return std::make_tuple(env, var, port_constr, vc_constr, cplex);
    }

    MPLinearSolution MPSolver::solve_lp(const ColumnPool &pool, const std::vector<PortWithType>& ports_with_equality) const {
        IloEnv env;
        IloNumVarArray var;
        IloRangeArray port_constr;
        IloRangeArray vc_constr;
        IloCplex cplex;

        auto np = prob->data.num_ports;

        std::tie(env, var, port_constr, vc_constr, cplex) = solve(pool, ports_with_equality, true);

        auto obj_value = cplex.getObjValue();

        IloNumArray values(env);

        cplex.getDuals(values, port_constr);

        auto port_duals = PortDuals();
        for(auto i = 1; i <= (values.getSize() / 2); i++) {
            std::shared_ptr<Port> p = prob->data.ports[i];
            port_duals.emplace(p, std::make_pair(values[i - 1], values[np - 1 + i - 1]));
        }

        cplex.getDuals(values, vc_constr);

        auto vc_duals = VcDuals();
        for(auto i = 0; i < values.getSize(); i++) {
            std::shared_ptr<VesselClass> vc = prob->data.vessel_classes[i];
            vc_duals.emplace(vc, values[i]);
        }

        cplex.getValues(values, var);

        auto variables = std::vector<double>();
        for(auto i = 0; i < values.getSize(); i++) {
            variables.push_back(values[i]);
        }

        values.end();
        env.end();

        return MPLinearSolution(obj_value, port_duals, vc_duals, variables);
    }

    MPIntegerSolution MPSolver::solve_mip(const ColumnPool &pool, const std::vector<PortWithType>& ports_with_equality) const {
        IloEnv env;
        IloNumVarArray var;
        IloRangeArray port_constr;
        IloRangeArray vc_constr;
        IloCplex cplex;

        std::tie(env, var, port_constr, vc_constr, cplex) = solve(pool, ports_with_equality, false);

        auto obj_value = cplex.getObjValue();

        IloNumArray values(env);

        cplex.getValues(values, var);

        auto variables = std::vector<double>();
        for(auto i = 0; i < values.getSize(); i++) {
            variables.push_back(values[i]);
        }

        values.end();
        env.end();

        return MPIntegerSolution(obj_value, variables);
    }

    bool MPSolver::should_row_have_equality(int row, const std::vector<PortWithType>& ports_with_equality) const {
        auto n = prob->data.num_ports;

        assert(row >= 1);
        assert(row <= 2 * (n-1));

        int port_id = row - 1;
        PortType pu = PortType::BOTH;

        if(port_id > n) {
            pu = PortType::DELIVERY;
            port_id -= (n - 1);
        } else {
            pu = PortType::PICKUP;
        }

        Port* port = prob->data.ports[port_id].get();

        return std::find(ports_with_equality.begin(), ports_with_equality.end(), std::make_pair(port, pu)) != ports_with_equality.end();
    }
}