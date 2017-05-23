// Copyright 2017 Global Phasing Ltd.
//
// Read mmcif (PDBx/mmCIF) file into a Structure from model.hh.

#ifndef GEMMI_MMCIF_HH_
#define GEMMI_MMCIF_HH_

#include <string>
#include <array>
#include <set>
#include <unordered_map>
#include "cif.hh"
#include "numb.hh"
#include "model.hh"

namespace gemmi {
namespace mol {

inline std::unordered_map<std::string, std::array<float,6>>
get_anisotropic_u(const cif::Block& block) {
  cif::TableView aniso_tab = block.find("_atom_site_anisotrop.",
                                        {"id", "U[1][1]", "U[2][2]", "U[3][3]",
                                         "U[1][2]", "U[1][3]", "U[2][3]"});
  std::unordered_map<std::string, std::array<float,6>> aniso_map;
  for (auto ani : aniso_tab)
    aniso_map[ani[0]] = {(float) cif::as_number(ani[1]),
                         (float) cif::as_number(ani[2]),
                         (float) cif::as_number(ani[3]),
                         (float) cif::as_number(ani[4]),
                         (float) cif::as_number(ani[5]),
                         (float) cif::as_number(ani[6])};
  return aniso_map;
}

inline Structure structure_from_cif_block(const cif::Block& block) {
  Structure st;
  st.name = block.name;

  // unit cell and symmetry
  cif::TableView cell = block.find("_cell.",
                                   {"length_a", "length_b", "length_c",
                                   "angle_alpha", "angle_beta", "angle_gamma"});
  if (cell.ok()) {
    auto c = cell.one();
    st.cell.set(c.as_num(0), c.as_num(1), c.as_num(2),
                c.as_num(3), c.as_num(4), c.as_num(5));
  }
  st.sg_hm = block.find_string("_symmetry.space_group_name_H-M");

  auto add_info = [&](std::string tag) {
    cif::TableView t = block.find(tag);
    if (t.length() >= 1)
      st.info[tag] = t[0].as_str(0);
  };
  add_info("_entry.id");
  add_info("_cell.Z_PDB");
  add_info("_exptl.method");
  add_info("_struct.title");
  // in pdbx/mmcif v5 date_original was replaced with a much longer tag
  std::string old_date_tag = "_database_PDB_rev.date_original";
  std::string new_date_tag
                      = "_pdbx_database_status.recvd_initial_deposition_date";
  add_info(old_date_tag);
  add_info(new_date_tag);
  if (st.info.count(old_date_tag) == 1 && st.info.count(new_date_tag) == 0)
    st.info[new_date_tag] = st.info[old_date_tag];
  add_info("_struct_keywords.pdbx_keywords");
  add_info("_struct_keywords.text");

  cif::TableView ncs_oper = block.find("_struct_ncs_oper.",
             {"matrix[1][1]", "matrix[1][2]", "matrix[1][3]",
              "matrix[2][1]", "matrix[2][2]", "matrix[2][3]",
              "matrix[3][1]", "matrix[3][2]", "matrix[3][3]",
              "vector[1]", "vector[2]", "vector[3]", "code"});
  for (auto op : ncs_oper) {
    Matrix33 mat{op.as_num(0), op.as_num(1), op.as_num(2),
                 op.as_num(3), op.as_num(4), op.as_num(5),
                 op.as_num(6), op.as_num(7), op.as_num(8)};
    Position vec{op.as_num(9), op.as_num(10), op.as_num(11)};
    bool given = (op.as_str(12) == "given");
    st.ncs.push_back({given, mat, vec});
  }

  auto aniso_map = get_anisotropic_u(block);

  // atom list
  enum { kId=0, kSymbol, kAtomId, kAltId, kCompId, kAsymId, kSeqId, kInsCode,
         kX, kY, kZ, kOcc, kBiso, kCharge, kAuthSeqId, kAuthAsymId, kModelNum };
  cif::TableView atom_table = block.find("_atom_site.",
                                         {"id",
                                          "type_symbol",
                                          "label_atom_id",
                                          "label_alt_id",
                                          "label_comp_id",
                                          "label_asym_id",
                                          "label_seq_id",
                                          "pdbx_PDB_ins_code",
                                          "Cartn_x",
                                          "Cartn_y",
                                          "Cartn_z",
                                          "occupancy",
                                          "B_iso_or_equiv",
                                          "pdbx_formal_charge",
                                          "auth_seq_id",
                                          "auth_asym_id",
                                          "pdbx_PDB_model_num"});
  Model *model = nullptr;
  Chain *chain = nullptr;
  Residue *resi = nullptr;
  for (auto row : atom_table) {
    if (!model || row[kModelNum] != model->name) {
      model = st.find_or_add_model(row[kModelNum]);
      chain = nullptr;
    }
    if (!chain || row[kAsymId] != chain->name) {
      chain = model->find_or_add_chain(row.as_str(kAsymId));
      chain->auth_name = row.as_str(kAuthAsymId);
      resi = nullptr;
    }
    int seq_id = cif::as_int(row[kSeqId], Residue::UnknownId);
    int auth_seq_id = cif::as_int(row[kAuthSeqId], Residue::UnknownId);
    char ins_code = cif::as_string(row[kInsCode])[0];
    if (!resi || seq_id != resi->seq_id || row[kCompId] != resi->name ||
        (seq_id == Residue::UnknownId &&
         (resi->auth_seq_id != auth_seq_id || resi->ins_code != ins_code))) {
      // the insertion code happens to be always a single letter
      assert(row[kInsCode].size() == 1);
      resi = chain->find_or_add_residue(seq_id, auth_seq_id, ins_code,
                                        cif::as_string(row[kCompId]));
    } else {
      assert(resi->auth_seq_id == auth_seq_id && resi->ins_code == ins_code);
    }
    Atom atom;
    atom.name = cif::as_string(row[kAtomId]);
    atom.altloc = cif::as_string(row[kAltId])[0];
    atom.charge = cif::is_null(row[kCharge]) ? 0 : cif::as_int(row[kCharge]);
    atom.element = Element(cif::as_string(row[kSymbol]));
    atom.pos.x = cif::as_number(row[kX]);
    atom.pos.y = cif::as_number(row[kY]);
    atom.pos.z = cif::as_number(row[kZ]);
    atom.occ = cif::as_number(row[kOcc], 1.0);
    atom.b_iso = cif::as_number(row[kBiso], 50.0);

    if (!aniso_map.empty()) {
      auto ani = aniso_map.find(row[kId]);
      if (ani != aniso_map.end()) {
        atom.u11 = ani->second[0];
        atom.u22 = ani->second[1];
        atom.u33 = ani->second[2];
        atom.u12 = ani->second[3];
        atom.u13 = ani->second[4];
        atom.u23 = ani->second[5];
      }
    }
    resi->atoms.emplace_back(atom);
  }

  for (const auto& row : block.find("_entity.", {"id", "type"})) {
    std::string id = row.as_str(0);
    EntityType etype = entity_type_from_string(row.as_str(1));
    st.entities.emplace_back(new Entity(id, etype));
  }

  for (const auto& row : block.find("_entity_poly_seq.",
                                    {"entity_id", "num", "mon_id"})) {
    Entity *ent = st.find_or_add_entity(row.as_str(0));
    ent->sequence.push_back({cif::as_int(row[1], -1), row.as_str(2)});
  }

  auto chain_to_entity = block.find("_struct_asym.", {"id", "entity_id"});
  for (Model& mod : st.models)
    for (Chain& ch : mod.chains)
      try {
        std::string ent_id = chain_to_entity.find_row(ch.name).as_str(1);
        ch.entity = st.find_or_add_entity(ent_id);
      } catch (std::runtime_error&) {  // maybe _struct_asym is missing
        ch.entity = nullptr;
      }
  st.finish();
  return st;
}

inline Structure read_atoms(const cif::Document& doc) {
  return structure_from_cif_block(doc.sole_block());
}

} // namespace mol
} // namespace gemmi
#endif
// vim:sw=2:ts=2:et