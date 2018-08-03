
#include "sage3basic.h"

#include "ROSE/Analysis/Template/instantiation-flow.hpp"
#include "ROSE/Analysis/Template/instantiation.hpp"
#include "ROSE/Analysis/Template/constraints.hpp"
#include "ROSE/Analysis/Template/element.hpp"
#include "ROSE/Analysis/Template/relation.hpp"

#include "MFB/Sage/driver.hpp"
#include "MFB/Sage/api.hpp"
#include "MFB/utils.hpp"

#include <sstream>

namespace ROSE { namespace Analysis { namespace Template {

void getPotentialFields(SgNonrealSymbol * sym, std::string prefix, std::map<SgNonrealSymbol *, std::string> & results) {
  std::string name = prefix;
  if (!prefix.empty())
    name += "::";

  name += sym->get_name();

  SgNonrealDecl * nrdecl = sym->get_declaration();
  ROSE_ASSERT(nrdecl != NULL);

  // TODO what if 'sym' represents a template (argument for a template template parameter)

  SgDeclarationScope * nrscope = nrdecl->get_nonreal_decl_scope();
  assert(nrscope != NULL);

  bool has_fields = false;

  SgSymbol * nrsym_ = nrscope->first_any_symbol();
  while (nrsym_ != NULL) {
    SgNonrealSymbol * nrsym = isSgNonrealSymbol(nrsym_);
    assert(nrsym != NULL);

    getPotentialFields(nrsym, name, results);

    has_fields = true;

    nrsym_ = nrscope->next_any_symbol();
  }

  if (!prefix.empty())
    results.insert(std::pair<SgNonrealSymbol *, std::string>(sym, name)); // No need to add the first one
}

////

void Instantiation::construct() {
  SgSymbol * template_symbol = NULL;
  SgTemplateParameterPtrList * tpl_params = NULL;
  SgTemplateArgumentPtrList * tpl_args = NULL;

  ::MFB::Utils::collect_symbol_template_info(symbol, template_symbol, nonreal_scope, tpl_params, tpl_args);
  
   if (template_symbol != NULL && template_symbol != symbol) {
    SgTemplateClassSymbol * tpl_sym_ = isSgTemplateClassSymbol(template_symbol);
    ROSE_ASSERT(tpl_sym_ != NULL);

    cannonical = TemplateInstantiation::build(tpl_sym_, this);
  } else if (template_symbol == symbol) {
    cannonical = dynamic_cast<TemplateInstantiation *>(this);
    assert(cannonical != NULL);
  }

  for (size_t i = 0; i < ( tpl_params != NULL ? tpl_params->size() : 0 ); i++) {
    SgTemplateParameter * param = (*tpl_params)[i];
    switch (param->get_parameterType()) {
      case SgTemplateParameter::type_parameter: {
        SgType * ptype = param->get_type();
        assert(ptype != NULL);
        SgNonrealType * nrptype = isSgNonrealType(ptype);
        assert(ptype != NULL);
        SgDeclarationStatement * pdecl = nrptype->get_declaration();
        assert(pdecl != NULL);
        SgNonrealDecl * nrpdecl = isSgNonrealDecl(pdecl);
        assert(nrpdecl != NULL);

        SgSymbol * psym = nrpdecl->search_for_symbol_from_symbol_table();
        assert(psym != NULL);
        SgNonrealSymbol * nrpsym = isSgNonrealSymbol(psym);
        assert(nrpsym != NULL);

        type_parameters.insert(std::pair<SgSymbol *, type_param_info_t>(nrpsym, type_param_info_t(i, field_map_t())));
        parameters_map.push_back(nrpsym);
        parameters_kind.push_back(e_type_parameter);

        getPotentialFields(nrpsym, "", type_parameters[nrpsym].second);

        break;
      }
      case SgTemplateParameter::nontype_parameter: {
        SgInitializedName * iname = param->get_initializedName();
        assert(iname != NULL);

        SgSymbol * isym = iname->search_for_symbol_from_symbol_table();
        assert(isym != NULL);
        SgVariableSymbol * visym = isSgVariableSymbol(isym);
        assert(visym != NULL);

        SgType * itype = iname->get_type();
        assert(itype != NULL);

        nontype_parameters.insert(std::pair<SgSymbol *, nontype_param_info_t>(visym, nontype_param_info_t(i, itype)));
        parameters_map.push_back(visym);
        parameters_kind.push_back(e_nontype_parameter);

        break;
      }
      case SgTemplateParameter::template_parameter: {
        SgDeclarationStatement * tdecl = param->get_templateDeclaration();
        assert(tdecl != NULL);
        SgNonrealDecl * nrtdecl = isSgNonrealDecl(tdecl);
        assert(nrtdecl != NULL);

        SgSymbol * tsym = nrtdecl->search_for_symbol_from_symbol_table();
        assert(tsym != NULL);
        SgNonrealSymbol * nrpsym = isSgNonrealSymbol(tsym);
        assert(nrpsym != NULL);

        template_parameters.insert(std::pair<SgSymbol *, template_param_info_t>(nrpsym, template_param_info_t(i, NULL)));
        parameters_map.push_back(nrpsym);
        parameters_kind.push_back(e_template_parameter);

        break;
      }
      case SgTemplateParameter::parameter_undefined: {
        assert(false);
      }
    }
  }

  for (size_t i = 0; i < ( tpl_args != NULL ? tpl_args->size() : 0 ); i++) {
    SgTemplateArgument * arg = (*tpl_args)[i];
    switch (arg->get_argumentType()) {
      case SgTemplateArgument::type_argument: {
        SgType * ptype = arg->get_type();
        assert(ptype != NULL);

        type_arguments.insert(std::pair<SgType *, type_arg_info_t>(ptype, type_arg_info_t(i, NULL)));
        arguments_map.push_back(ptype);
        arguments_kind.push_back(e_type_argument);

        break;
      }
      case SgTemplateArgument::nontype_argument: {
        SgExpression * expr = arg->get_expression();
        assert(expr != NULL); // FIXME: SgInitializedName * iname = arg->get_initializedName();

        nontype_arguments.insert(std::pair<SgExpression *, nontype_arg_info_t>(expr, nontype_arg_info_t(i, NULL)));
        arguments_map.push_back(expr);
        arguments_kind.push_back(e_nontype_argument);

        break;
      }
      case SgTemplateArgument::template_template_argument: {
        SgDeclarationStatement * tdecl = arg->get_templateDeclaration();
        assert(tdecl != NULL);
        SgNonrealDecl * nrtdecl = isSgNonrealDecl(tdecl);
        assert(nrtdecl != NULL);

        SgSymbol * tsym = nrtdecl->search_for_symbol_from_symbol_table();
        assert(tsym != NULL);
        SgNonrealSymbol * nrpsym = isSgNonrealSymbol(tsym);
        assert(nrpsym != NULL);

        template_arguments.insert(std::pair<SgSymbol *, template_arg_info_t>(nrpsym, template_arg_info_t(i, NULL)));
        arguments_map.push_back(nrpsym);
        arguments_kind.push_back(e_template_argument);

        break;
      }
      case SgTemplateArgument::start_of_pack_expansion_argument: {
        assert(false); // TODO
        break;
      }
      case SgTemplateArgument::argument_undefined: {
        assert(false);
      }
    }
  }
}

void NonrealInstantiation::construct() {
  Instantiation::construct();
  assert(cannonical != NULL);
}
    
void TemplateInstantiation::construct() {
  Instantiation::construct();

  if (cannonical != NULL && cannonical != this) {
    cannonical->specializations[0].insert(this);
  }

  size_t i = 0;
  SgSymbol * sym = nonreal_scope != NULL ? nonreal_scope->first_any_symbol() : NULL;
  while (sym != NULL) {
    if (
      nontype_parameters.find(sym) == nontype_parameters.end() &&
      type_parameters.find(sym) == type_parameters.end() &&
      template_parameters.find(sym) == template_parameters.end()
    ) {
      SgNonrealSymbol * nrsym = isSgNonrealSymbol(sym);
      assert(nrsym != NULL);

      instantiations.insert(NonrealInstantiation::build(nrsym, this));
    }

    sym = nonreal_scope->next_any_symbol();
  }

  SgClassSymbol * csym = isSgClassSymbol(symbol);
  assert(csym != NULL);
  SgClassDeclaration * cdecl = isSgClassDeclaration(csym->get_declaration());
  assert(cdecl != NULL);
  SgDeclarationStatement * decl = cdecl->get_definingDeclaration();
  if (decl != NULL) {
    cdecl = isSgClassDeclaration(decl);
    assert(cdecl != NULL);
    assert(cdecl->get_definition() != NULL);
    is_defined = true;
  }
}

////

Instantiation::Instantiation(SgSymbol * sym) :
  symbol(sym),
  cannonical(NULL),
  nonreal_scope(NULL),
  nontype_parameters(),
  type_parameters(),
  template_parameters(),
  parameters_map(),
  parameters_kind(),
  nontype_arguments(),
  type_arguments(),
  template_arguments(),
  arguments_map(),
  arguments_kind(),
  sources()
{}

Instantiation::~Instantiation() {}

NonrealInstantiation::NonrealInstantiation(SgSymbol * symbol__) :
  Instantiation(symbol__)
{}

TemplateInstantiation::TemplateInstantiation(SgSymbol * symbol__) :
  Instantiation(symbol__),
  specializations(),
  instantiations(),
  is_defined(false)
{
  specializations.push_back(std::set<TemplateInstantiation *>());
}

////

NonrealInstantiation * NonrealInstantiation::build(SgSymbol * symbol, Instantiation * source) {
  NonrealInstantiation * result = NULL;
  if (all.find(symbol) == all.end()) {
    result = new NonrealInstantiation(symbol);
    all.insert(std::pair<SgSymbol *, Instantiation *>(symbol, result));
    result->construct();
  } else {
    result = dynamic_cast<NonrealInstantiation*>(all[symbol]);
    assert(result != NULL);
  }

  if (source != NULL) {
    result->sources.insert(source);
  }

  return result;
}

TemplateInstantiation * TemplateInstantiation::build(SgSymbol * symbol, Instantiation * source) {
  TemplateInstantiation * result = NULL;
  if (all.find(symbol) == all.end()) {
    result = new TemplateInstantiation(symbol);
    all.insert(std::pair<SgSymbol *, Instantiation *>(symbol, result));
    result->construct();
  } else {
    result = dynamic_cast<TemplateInstantiation*>(all[symbol]);
    assert(result != NULL);
  }

  if (source != NULL) {
    result->sources.insert(source);
  }

  return result;
}

////

std::string NonrealInstantiation::getGraphVizTag() const {
  std::ostringstream oss;
  oss << "ni_" << std::hex << symbol;
  return oss.str();
}

std::string TemplateInstantiation::getGraphVizTag() const {
  std::ostringstream oss;
  oss << "ti_" << std::hex << symbol;
  return oss.str();
}

std::string Instantiation::getGraphVizLabel() const {
  std::ostringstream oss;
  oss << symbol->class_name();
  oss << "\\n";
  oss << symbol->get_name().getString();
  oss << "\\n\\n";
  oss << getGraphVizLabelConstraints();
  return oss.str();
}

void Instantiation::toGraphViz(std::ostream & out) const {
  out << getGraphVizTag() << "[label=\"" << getGraphVizLabel() << "\", shape=\"" << getGraphVizShape() << "\", color=\"" << getGraphVizNodeColor() << "\"];" << std::endl;
}

std::string NonrealInstantiation::getGraphVizShape() const {
  return std::string("diamond");
}

std::string TemplateInstantiation::getGraphVizShape() const {
  return std::string("ellipse");
}

std::string NonrealInstantiation::getGraphVizLabelConstraints() const {
  std::ostringstream oss;
  return oss.str();
}

std::string TemplateInstantiation::getGraphVizLabelConstraints() const {
  std::ostringstream oss;
  for (auto it = field_constraints.begin(); it != field_constraints.end(); it++) {
    SgNonrealSymbol * nrsym = isSgNonrealSymbol(parameters_map[it->second.first]);
    assert(nrsym != NULL);

    oss << nrsym->get_name().getString() << " (" << it->second.first << ") :: " << it->second.second.substr(it->second.second.find("::") + 2) << "\\n";
  }
  return oss.str();
}

std::string NonrealInstantiation::getGraphVizNodeColor() const {
  return std::string("blue");
}

std::string TemplateInstantiation::getGraphVizNodeColor() const {
  if (isSgTemplateClassSymbol(symbol))
    return std::string("red");
  else
    return std::string("green");
}

////

void NonrealInstantiation::finalize() {
  assert(cannonical != NULL);

  for (size_t i = 0; i < cannonical->specializations.size(); i++)
    for (auto it = cannonical->specializations[i].begin(); it != cannonical->specializations[i].end(); it++)
      SpecializationConstraints::build(this, *it, i);

  if (cannonical->is_defined)
    SpecializationConstraints::build(this, cannonical, cannonical->specializations.size());
}

void TemplateInstantiation::finalize() {
//std::cerr << "TemplateInstantiation::finalize(this = " << this << "):" << std::endl;
  for (auto it = type_parameters.begin(); it != type_parameters.end(); it++) {
//  std::cerr << " -- type_parameters[" << it->first << "] = " << it->second.first << std::endl;
    for (auto fit = it->second.second.begin(); fit != it->second.second.end(); fit++) {
//    std::cerr << "   -- (" << fit->first << " , " << fit->second << " )" << std::endl;
      field_constraints.insert(std::pair<SgNonrealSymbol *, std::pair< size_t , std::string > >(fit->first, std::pair<size_t, std::string>( it->second.first , fit->second )));
    }
  }

  // TODO:
  //   -- compute partial ordering for `specializations`

//if (cannonical != NULL)
//  Relation::build(this, cannonical);

  for (auto it = instantiations.begin(); it != instantiations.end(); it++)
    InstantiationConstraints::build(this, *it);
}


} } }

