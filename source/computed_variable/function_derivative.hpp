//******************************************************************************
// FILE : function_derivative.hpp
//
// LAST MODIFIED : 23 June 2004
//
// DESCRIPTION :
//==============================================================================
#if !defined (__FUNCTION_DERIVATIVE_HPP__)
#define __FUNCTION_DERIVATIVE_HPP__

#include <list>
#include "computed_variable/function.hpp"

class Function_derivative;

typedef boost::intrusive_ptr<Function_derivative> Function_derivative_handle;

class Function_derivative : public Function
//******************************************************************************
// LAST MODIFIED : 23 June 2004
//
// DESCRIPTION :
// A derivative of another function.
//
// No storage for result and so can't be inverted.
// ???DB.  Could compose with a Function_matrix and then would be invertable?
//==============================================================================
{
	friend class Function_variable_derivative;
	friend class Function_variable_iterator_representation_atomic_derivative;
	public:
		// constructor
		Function_derivative(const Function_variable_handle& dependent_variable,
			std::list<Function_variable_handle>& independent_variables);
		// destructor
		~Function_derivative();
	// inherited
	public:
		string_handle get_string_representation();
		Function_variable_handle input();
		Function_variable_handle output();
	private:
		Function_handle evaluate(Function_variable_handle atomic_variable);
		bool evaluate_derivative(Scalar& derivative,
			Function_variable_handle atomic_variable,
			std::list<Function_variable_handle>& atomic_independent_variables);
		bool set_value(Function_variable_handle atomic_variable,
			Function_variable_handle atomic_value);
		Function_handle get_value(Function_variable_handle atomic_variable);
	private:
		// copy constructor
		Function_derivative(const Function_derivative&);
		// assignment
		Function_derivative& operator=(const Function_derivative&);
	private:
		Function_variable_handle dependent_variable_private;
		std::list<Function_variable_handle> independent_variables_private;
};

#endif /* !defined (__FUNCTION_DERIVATIVE_HPP__) */
