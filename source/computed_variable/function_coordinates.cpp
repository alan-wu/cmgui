//******************************************************************************
// FILE : function_coordinates.cpp
//
// LAST MODIFIED : 13 July 2004
//
// DESCRIPTION :
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
//???DB.
// Function_variable_iterator_representation_atomic_rectangular_cartesian
// is a copy of
// Function_variable_iterator_representation_atomic_finite_element
// and
// Function_variable_rectangular_cartesian
// is a copy of
// Function_variable_finite_element
// How to abstract?
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
//==============================================================================

#include <sstream>

#include "computed_variable/function_coordinates.hpp"
#include "computed_variable/function_matrix.hpp"
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#include "computed_variable/function_variable.hpp"
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#include "computed_variable/function_variable_composite.hpp"
#include "computed_variable/function_variable_matrix.hpp"
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#include "computed_variable/function_variable_value_scalar.hpp"

// module typedefs
// ===============

#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
typedef boost::intrusive_ptr< Function_variable_matrix<Scalar> >
	Function_variable_matrix_handle;
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)

// module classes
// ==============

#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
// forward declaration so that can use _handle
class Function_variable_rectangular_cartesian;
typedef boost::intrusive_ptr<Function_variable_rectangular_cartesian>
	Function_variable_rectangular_cartesian_handle;

// class Function_variable_iterator_representation_atomic_rectangular_cartesian
// ----------------------------------------------------------------------------

class Function_variable_iterator_representation_atomic_rectangular_cartesian:
	public Function_variable_iterator_representation
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	public:
		// constructor
		Function_variable_iterator_representation_atomic_rectangular_cartesian(
			const bool begin,Function_variable_rectangular_cartesian_handle variable);
		// a "virtual" constructor
		Function_variable_iterator_representation *clone();
	protected:
		// destructor
		~Function_variable_iterator_representation_atomic_rectangular_cartesian();
	private:
		// increment
		void increment();
		// decrement
		void decrement();
		// equality
		bool equality(
			const Function_variable_iterator_representation* representation);
		// dereference
		Function_variable_handle dereference() const;
	private:
		// copy constructor
		Function_variable_iterator_representation_atomic_rectangular_cartesian(const
			Function_variable_iterator_representation_atomic_rectangular_cartesian&);
	private:
		Function_variable_rectangular_cartesian_handle atomic_variable,variable;
};

static bool Function_variable_rectangular_cartesian_set_scalar_function(
	Scalar& value,const Function_variable_handle variable);


// class Function_variable_rectangular_cartesian
// ---------------------------------------------

class Function_variable_rectangular_cartesian : public Function_variable
//******************************************************************************
// LAST MODIFIED : 30 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	friend class Function_prolate_spheroidal_to_rectangular_cartesian;
	friend class
		Function_variable_iterator_representation_atomic_rectangular_cartesian;
	friend bool Function_variable_rectangular_cartesian_set_scalar_function(
		Scalar& value,const Function_variable_handle variable);
	public:
		// constructors.  A zero component_number indicates all components
		Function_variable_rectangular_cartesian(
			const Function_prolate_spheroidal_to_rectangular_cartesian_handle&
			function_prolate_spheroidal_to_rectangular_cartesian,
			Function_size_type component_number=0):
			Function_variable(function_prolate_spheroidal_to_rectangular_cartesian),
			component_number(component_number)
		{
			if (function_prolate_spheroidal_to_rectangular_cartesian&&is_atomic())
			{
				value_private=Function_variable_value_handle(
					new Function_variable_value_scalar(
					Function_variable_rectangular_cartesian_set_scalar_function));
			}
		};
		Function_variable_rectangular_cartesian(
			const Function_prolate_spheroidal_to_rectangular_cartesian_handle&
			function_prolate_spheroidal_to_rectangular_cartesian,
			const std::string component_name):
			Function_variable(function_prolate_spheroidal_to_rectangular_cartesian),
			component_number(0)
		{
			if (function_prolate_spheroidal_to_rectangular_cartesian)
			{
				Function_size_type local_component_number;

				local_component_number=0;
				if ((std::string("x")==component_name)||
					(std::string("X")==component_name))
				{
					local_component_number=1;
				}
				else if ((std::string("y")==component_name)||
					(std::string("Y")==component_name))
				{
					local_component_number=2;
				}
				else if ((std::string("z")==component_name)||
					(std::string("Z")==component_name))
				{
					local_component_number=3;
				}
				if ((0<local_component_number)&&(local_component_number<=
					function_prolate_spheroidal_to_rectangular_cartesian->
					number_of_components()))
				{
					component_number=local_component_number;
					value_private=Function_variable_value_handle(
						new Function_variable_value_scalar(
						Function_variable_rectangular_cartesian_set_scalar_function));
				}
			}
		};
		// destructor
		~Function_variable_rectangular_cartesian(){};
	// inherited
	public:
		Function_variable_handle clone() const
		{
			return (Function_variable_rectangular_cartesian_handle(
				new Function_variable_rectangular_cartesian(*this)));
		};
		string_handle get_string_representation()
		{
			string_handle return_string(0);

			if (return_string=new std::string)
			{
				std::ostringstream out;

				out << "rectangular cartesian";
				switch (component_number)
				{
					case 0:
					{
						// do nothing
					} break;
					case 1:
					{
						out << ".x";
					} break;
					case 2:
					{
						out << ".y";
					} break;
					case 3:
					{
						out << ".z";
					} break;
					default:
					{
						out << "[" << component_number << "]";
					} break;
				}
				*return_string=out.str();
			}

			return (return_string);
		};
		virtual Function_variable_iterator begin_atomic() const
		{
			return (Function_variable_iterator(new
				Function_variable_iterator_representation_atomic_rectangular_cartesian(
				true,Function_variable_rectangular_cartesian_handle(
				const_cast<Function_variable_rectangular_cartesian*>(this)))));
		};
		virtual Function_variable_iterator end_atomic() const
		{
			return (Function_variable_iterator(new
				Function_variable_iterator_representation_atomic_rectangular_cartesian(
				false,Function_variable_rectangular_cartesian_handle(
				const_cast<Function_variable_rectangular_cartesian*>(this)))));
		};
		std::reverse_iterator<Function_variable_iterator> rbegin_atomic() const
		{
			return (std::reverse_iterator<Function_variable_iterator>(new
				Function_variable_iterator_representation_atomic_rectangular_cartesian(
				false,Function_variable_rectangular_cartesian_handle(
				const_cast<Function_variable_rectangular_cartesian*>(this)))));
		};
		std::reverse_iterator<Function_variable_iterator> rend_atomic() const
		{
			return (std::reverse_iterator<Function_variable_iterator>(new
				Function_variable_iterator_representation_atomic_rectangular_cartesian(
				true,Function_variable_rectangular_cartesian_handle(
				const_cast<Function_variable_rectangular_cartesian*>(this)))));
		};
		Function_size_type number_differentiable()
		{
			Function_prolate_spheroidal_to_rectangular_cartesian_handle
				function_prolate_spheroidal_to_rectangular_cartesian=
				boost::dynamic_pointer_cast<
				Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
				function());
			Function_size_type result;

			result=0;
			if (function_prolate_spheroidal_to_rectangular_cartesian)
			{
				Function_size_type number_of_components=
					function_prolate_spheroidal_to_rectangular_cartesian->
					number_of_components();
				if (component_number<=number_of_components)
				{
					if (0==component_number)
					{
						result=number_of_components;
					}
					else
					{
						result=1;
					}
				}
			}

			return (result);
		};
		bool equality_atomic(const Function_variable_handle& variable) const
		{
			bool result;
			Function_prolate_spheroidal_to_rectangular_cartesian_handle
				function_prolate_spheroidal_to_rectangular_cartesian=
				boost::dynamic_pointer_cast<
				Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
				function());
			Function_variable_rectangular_cartesian_handle
				variable_rectangular_cartesian;

			result=false;
			if (variable_rectangular_cartesian=boost::dynamic_pointer_cast<
				Function_variable_rectangular_cartesian,Function_variable>(variable))
			{
				if ((variable_rectangular_cartesian->function()==
					function_prolate_spheroidal_to_rectangular_cartesian)&&
					(variable_rectangular_cartesian->component_number==component_number))
				{
					result=true;
				}
			}

			return (result);
		};
	private:
		bool is_atomic()
		{
			bool result;

			result=false;
			if (this&&(0!=component_number))
			{
				result=true;
			}

			return (result);
		};
	private:
		// copy constructor
		Function_variable_rectangular_cartesian(const
			Function_variable_rectangular_cartesian& variable_rectangular_cartesian):
			Function_variable(variable_rectangular_cartesian),
			component_number(variable_rectangular_cartesian.component_number){};
		// assignment
		Function_variable_rectangular_cartesian& operator=(
			const Function_variable_rectangular_cartesian&);
	private:
		Function_size_type component_number;
};

static bool Function_variable_rectangular_cartesian_set_scalar_function(
	Scalar& value,const Function_variable_handle variable)
{
	bool result;
	Function_variable_rectangular_cartesian_handle variable_rectangular_cartesian;
	Function_prolate_spheroidal_to_rectangular_cartesian_handle
		function_prolate_spheroidal_to_rectangular_cartesian;

	result=false;
	if ((variable_rectangular_cartesian=boost::dynamic_pointer_cast<
		Function_variable_rectangular_cartesian,Function_variable>(variable))&&
		(variable_rectangular_cartesian->is_atomic)()&&
		(function_prolate_spheroidal_to_rectangular_cartesian=
		boost::dynamic_pointer_cast<
		Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
		variable_rectangular_cartesian->function())))
	{
		switch (variable_rectangular_cartesian->component_number)
		{
			case 1:
			{
				value=(function_prolate_spheroidal_to_rectangular_cartesian->x_value)();
				result=true;
			} break;
			case 2:
			{
				value=(function_prolate_spheroidal_to_rectangular_cartesian->y_value)();
				result=true;
			} break;
			case 3:
			{
				value=(function_prolate_spheroidal_to_rectangular_cartesian->z_value)();
				result=true;
			} break;
		}
	}

	return (result);
}


// class Function_variable_iterator_representation_atomic_rectangular_cartesian
// ----------------------------------------------------------------------------

Function_variable_iterator_representation_atomic_rectangular_cartesian::
	Function_variable_iterator_representation_atomic_rectangular_cartesian(
	const bool begin,Function_variable_rectangular_cartesian_handle variable):
	atomic_variable(0),variable(variable)
//******************************************************************************
// LAST MODIFIED : 30 June 2004
//
// DESCRIPTION :
// Constructor.  If <begin> then the constructed iterator points to the first
// atomic variable, otherwise it points to one past the last atomic variable.
//==============================================================================
{
	if (begin&&variable)
	{
		Function_prolate_spheroidal_to_rectangular_cartesian_handle
			function_prolate_spheroidal_to_rectangular_cartesian=
			boost::dynamic_pointer_cast<
			Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
			variable->function());

		if (function_prolate_spheroidal_to_rectangular_cartesian&&
			(atomic_variable=boost::dynamic_pointer_cast<
			Function_variable_rectangular_cartesian,Function_variable>(
			variable->clone())))
		{
			if (0==variable->component_number)
			{
				if (0<(function_prolate_spheroidal_to_rectangular_cartesian->
					number_of_components)())
				{
					atomic_variable->component_number=1;
				}
				else
				{
					// end
					atomic_variable=0;
				}
			}
			else
			{
				if (variable->component_number>(
					function_prolate_spheroidal_to_rectangular_cartesian->
					number_of_components)())
				{
					// end
					atomic_variable=0;
				}
			}
			if (atomic_variable)
			{
				atomic_variable->value_private=Function_variable_value_handle(
					new Function_variable_value_scalar(
					Function_variable_rectangular_cartesian_set_scalar_function));
			}
		}
	}
}

Function_variable_iterator_representation
	*Function_variable_iterator_representation_atomic_rectangular_cartesian::
	clone()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	Function_variable_iterator_representation *result;

	result=0;
	if (this)
	{
		result=new
			Function_variable_iterator_representation_atomic_rectangular_cartesian(
			*this);
	}

	return (result);
}

Function_variable_iterator_representation_atomic_rectangular_cartesian::
	~Function_variable_iterator_representation_atomic_rectangular_cartesian()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
// Destructor.
//==============================================================================
{
	// do nothing
}

void Function_variable_iterator_representation_atomic_rectangular_cartesian::
	increment()
//******************************************************************************
// LAST MODIFIED : 30 June 2004
//
// DESCRIPTION :
// Increments the iterator to the next atomic variable.  NULL <atomic_variable>
// is the end iterator.
//==============================================================================
{
	if (atomic_variable&&variable)
	{
		Function_prolate_spheroidal_to_rectangular_cartesian_handle
			function_prolate_spheroidal_to_rectangular_cartesian=
			boost::dynamic_pointer_cast<
			Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
			variable->function());

		if (function_prolate_spheroidal_to_rectangular_cartesian&&
			(0==variable->component_number))
		{
			if (atomic_variable->component_number<
				(function_prolate_spheroidal_to_rectangular_cartesian->
				number_of_components)())
			{
				(atomic_variable->component_number)++;
			}
			else
			{
				// end
				atomic_variable=0;
			}
		}
		else
		{
			// end
			atomic_variable=0;
		}
	}
}

void Function_variable_iterator_representation_atomic_rectangular_cartesian::
	decrement()
//******************************************************************************
// LAST MODIFIED : 23 June 2004
//
// DESCRIPTION :
// Decrements the iterator to the next atomic variable.  NULL <atomic_variable>
// is the end iterator.
//==============================================================================
{
	if (atomic_variable)
	{
		if (0==variable->component_number)
		{
			if (1<atomic_variable->component_number)
			{
				(atomic_variable->component_number)--;
			}
			else
			{
				// end
				atomic_variable=0;
			}
		}
		else
		{
			// end
			atomic_variable=0;
		}
	}
	else
	{
		if (variable)
		{
			Function_prolate_spheroidal_to_rectangular_cartesian_handle
				function_prolate_spheroidal_to_rectangular_cartesian=
				boost::dynamic_pointer_cast<
				Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
				variable->function());

			if (function_prolate_spheroidal_to_rectangular_cartesian&&
				(atomic_variable=boost::dynamic_pointer_cast<
				Function_variable_rectangular_cartesian,Function_variable>(
				variable->clone())))
			{
				if (0==variable->component_number)
				{
					atomic_variable->component_number=
						(function_prolate_spheroidal_to_rectangular_cartesian->
						number_of_components)();
					if (0==atomic_variable->component_number)
					{
						// end
						atomic_variable=0;
					}
				}
				else
				{
					if (variable->component_number>
						(function_prolate_spheroidal_to_rectangular_cartesian->
						number_of_components)())
					{
						// end
						atomic_variable=0;
					}
				}
				if (atomic_variable)
				{
					atomic_variable->value_private=Function_variable_value_handle(
						new Function_variable_value_scalar(
						Function_variable_rectangular_cartesian_set_scalar_function));
				}
			}
		}
	}
}

bool Function_variable_iterator_representation_atomic_rectangular_cartesian::
	equality(const Function_variable_iterator_representation * representation)
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
// Tests <*this> and <*representation> for equality.
//==============================================================================
{
	bool result;
	const Function_variable_iterator_representation_atomic_rectangular_cartesian
		*representation_rectangular_cartesian=dynamic_cast<const
		Function_variable_iterator_representation_atomic_rectangular_cartesian *>(
		representation);

	result=false;
	if (representation_rectangular_cartesian)
	{
		if (((0==atomic_variable)&&
			(0==representation_rectangular_cartesian->atomic_variable))||
			(atomic_variable&&
			(representation_rectangular_cartesian->atomic_variable)&&
			(*atomic_variable==
			*(representation_rectangular_cartesian->atomic_variable))))
		{
			result=true;
		}
	}

	return (result);
}

Function_variable_handle
	Function_variable_iterator_representation_atomic_rectangular_cartesian::
	dereference() const
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
// Returns the atomic variable for the iterator.
//==============================================================================
{
	return (atomic_variable);
}

Function_variable_iterator_representation_atomic_rectangular_cartesian::
	Function_variable_iterator_representation_atomic_rectangular_cartesian(const
	Function_variable_iterator_representation_atomic_rectangular_cartesian&
	representation):Function_variable_iterator_representation(),
	atomic_variable(0),variable(representation.variable)
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
// Copy constructor.
//==============================================================================
{
	if (representation.atomic_variable)
	{
		atomic_variable=boost::dynamic_pointer_cast<
			Function_variable_rectangular_cartesian,Function_variable>(
			(representation.atomic_variable)->clone());
	}
}
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)

// class Function_variable_matrix_rectangular_cartesian
// ----------------------------------------------------

// forward declaration so that can use _handle
class Function_variable_matrix_rectangular_cartesian;
typedef boost::intrusive_ptr<Function_variable_matrix_rectangular_cartesian>
	Function_variable_matrix_rectangular_cartesian_handle;

class Function_variable_matrix_rectangular_cartesian :
	public Function_variable_matrix<Scalar>
//******************************************************************************
// LAST MODIFIED : 13 July 2004
//
// DESCRIPTION :
//==============================================================================
{
	friend class Function_prolate_spheroidal_to_rectangular_cartesian;
	public:
		// constructors.  A zero component_number indicates all components
		Function_variable_matrix_rectangular_cartesian(
			const Function_prolate_spheroidal_to_rectangular_cartesian_handle&
			function_prolate_spheroidal_to_rectangular_cartesian,
			Function_size_type component_number=0):Function_variable_matrix<Scalar>(
			function_prolate_spheroidal_to_rectangular_cartesian,component_number,1)
			{};
		Function_variable_matrix_rectangular_cartesian(
			const Function_prolate_spheroidal_to_rectangular_cartesian_handle&
			function_prolate_spheroidal_to_rectangular_cartesian,
			const std::string component_name):Function_variable_matrix<Scalar>(
			function_prolate_spheroidal_to_rectangular_cartesian,0,1)
		{
			if (function_prolate_spheroidal_to_rectangular_cartesian)
			{
				Function_size_type local_component_number;

				local_component_number=0;
				if ((std::string("x")==component_name)||
					(std::string("X")==component_name))
				{
					local_component_number=1;
				}
				else if ((std::string("y")==component_name)||
					(std::string("Y")==component_name))
				{
					local_component_number=2;
				}
				else if ((std::string("z")==component_name)||
					(std::string("Z")==component_name))
				{
					local_component_number=3;
				}
				if ((0<local_component_number)&&(local_component_number<=
					function_prolate_spheroidal_to_rectangular_cartesian->
					number_of_components()))
				{
					row=local_component_number;
					value_private=Function_variable_value_handle(
						new Function_variable_value_specific<Scalar>(
						Function_variable_matrix_set_value_function<Scalar>));
				}
			}
		};
		// destructor
		~Function_variable_matrix_rectangular_cartesian(){};
	// inherited
	public:
		Function_variable_handle clone() const
		{
			return (Function_variable_matrix_rectangular_cartesian_handle(
				new Function_variable_matrix_rectangular_cartesian(*this)));
		};
		string_handle get_string_representation()
		{
			string_handle return_string(0);

			if (return_string=new std::string)
			{
				std::ostringstream out;

				out << "rectangular cartesian";
				switch (row)
				{
					case 0:
					{
						// do nothing
					} break;
					case 1:
					{
						out << ".x";
					} break;
					case 2:
					{
						out << ".y";
					} break;
					case 3:
					{
						out << ".z";
					} break;
					default:
					{
						out << "[" << row << "]";
					} break;
				}
				*return_string=out.str();
			}

			return (return_string);
		};
		Function_variable_matrix_handle operator()(
			Function_size_type row,Function_size_type column)
		{
			Function_variable_matrix_handle result(0);

			if ((row<=number_of_rows())&&(column<=number_of_columns()))
			{
				result=Function_variable_matrix_handle(
					new Function_variable_matrix_rectangular_cartesian(
					boost::dynamic_pointer_cast<
					Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
					function_private),row));
			}

			return (result);
		};
		Function_size_type number_of_rows() const
		{
			Function_prolate_spheroidal_to_rectangular_cartesian_handle
				function_prolate_spheroidal_to_rectangular_cartesian;
			Function_size_type result;

			result=0;
			if (this&&(function_prolate_spheroidal_to_rectangular_cartesian=
				boost::dynamic_pointer_cast<
				Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
				function())))
			{
				result=function_prolate_spheroidal_to_rectangular_cartesian->
					number_of_components();
			}

			return (result);
		};
		Function_size_type number_of_columns() const
		{
			return (1);
		};
		bool get_entry(Scalar& value) const
		{
			bool result;
			Function_prolate_spheroidal_to_rectangular_cartesian_handle
				function_prolate_spheroidal_to_rectangular_cartesian;

			result=false;
			if (this&&(1==column)&&
				(function_prolate_spheroidal_to_rectangular_cartesian=
				boost::dynamic_pointer_cast<
				Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
				function())))
			{
				switch (row)
				{
					case 1:
					{
						value=
							(function_prolate_spheroidal_to_rectangular_cartesian->x_value)();
						result=true;
					} break;
					case 2:
					{
						value=
							(function_prolate_spheroidal_to_rectangular_cartesian->y_value)();
						result=true;
					} break;
					case 3:
					{
						value=
							(function_prolate_spheroidal_to_rectangular_cartesian->z_value)();
						result=true;
					} break;
				}
			}

			return (result);
		};
	private:
		// copy constructor
		Function_variable_matrix_rectangular_cartesian(const
			Function_variable_matrix_rectangular_cartesian&
			variable_rectangular_cartesian):
			Function_variable_matrix<Scalar>(variable_rectangular_cartesian){};
};
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)


#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
// forward declaration so that can use _handle
class Function_variable_prolate_spheroidal;
typedef boost::intrusive_ptr<Function_variable_prolate_spheroidal>
	Function_variable_prolate_spheroidal_handle;


// class Function_variable_iterator_representation_atomic_prolate_spheroidal
// -------------------------------------------------------------------------

class Function_variable_iterator_representation_atomic_prolate_spheroidal :
	public Function_variable_iterator_representation
//******************************************************************************
// LAST MODIFIED : 4 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	public:
		// constructor
		Function_variable_iterator_representation_atomic_prolate_spheroidal(
			const bool begin,Function_variable_prolate_spheroidal_handle variable);
		// a "virtual" constructor
		Function_variable_iterator_representation *clone();
	protected:
		// destructor
		~Function_variable_iterator_representation_atomic_prolate_spheroidal();
	private:
		// increment
		void increment();
		// decrement
		void decrement();
		// equality
		bool equality(
			const Function_variable_iterator_representation* representation);
		// dereference
		Function_variable_handle dereference() const;
	private:
		// copy constructor
		Function_variable_iterator_representation_atomic_prolate_spheroidal(const
			Function_variable_iterator_representation_atomic_prolate_spheroidal&);
	private:
		Function_variable_prolate_spheroidal_handle atomic_variable,variable;
};

static bool Function_variable_prolate_spheroidal_set_scalar_function(
	Scalar& value,const Function_variable_handle variable);


// class Function_variable_prolate_spheroidal
// ------------------------------------------

class Function_variable_prolate_spheroidal : public Function_variable
//******************************************************************************
// LAST MODIFIED : 30 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	friend class Function_prolate_spheroidal_to_rectangular_cartesian;
	friend
		class Function_variable_iterator_representation_atomic_prolate_spheroidal;
	friend bool Function_variable_prolate_spheroidal_set_scalar_function(
		Scalar& value,const Function_variable_handle variable);
	public:
		// constructor
		Function_variable_prolate_spheroidal(
			const Function_prolate_spheroidal_to_rectangular_cartesian_handle&
			function_prolate_spheroidal_to_rectangular_cartesian,bool lambda=true,
			bool mu=true,bool theta=true,bool focus=false):
			Function_variable(function_prolate_spheroidal_to_rectangular_cartesian),
			focus(focus),lambda(lambda),mu(mu),theta(theta)
		{
			if (function_prolate_spheroidal_to_rectangular_cartesian&&is_atomic())
			{
				value_private=Function_variable_value_handle(
					new Function_variable_value_scalar(
					Function_variable_prolate_spheroidal_set_scalar_function));
			}
		};
		~Function_variable_prolate_spheroidal(){};
	// inherited
	public:
		Function_variable_handle clone() const
		{
			return (Function_variable_handle(new Function_variable_prolate_spheroidal(
				*this)));
		};
		string_handle get_string_representation()
		{
			string_handle return_string(0);

			if (return_string=new std::string)
			{
				bool comma;
				std::ostringstream out;

				out << "(";
				comma=false;
				if (lambda)
				{
					out << "lambda";
					comma=true;
				}
				if (mu)
				{
					if (comma)
					{
						out << ",";
					}
					out << "mu";
					comma=true;
				}
				if (theta)
				{
					if (comma)
					{
						out << ",";
					}
					out << "theta";
					comma=true;
				}
				if (focus)
				{
					if (comma)
					{
						out << ",";
					}
					out << "focus";
					comma=true;
				}
				out << ")";
				*return_string=out.str();
			}

			return (return_string);
		};
		Function_variable_iterator begin_atomic() const
		{
			return (Function_variable_iterator(
				new Function_variable_iterator_representation_atomic_prolate_spheroidal(
				true,Function_variable_prolate_spheroidal_handle(const_cast<
				Function_variable_prolate_spheroidal*>(this)))));
		};
		Function_variable_iterator end_atomic() const
		{
			return (Function_variable_iterator(
				new Function_variable_iterator_representation_atomic_prolate_spheroidal(
				false,Function_variable_prolate_spheroidal_handle(const_cast<
				Function_variable_prolate_spheroidal*>(this)))));
		};
		std::reverse_iterator<Function_variable_iterator> rbegin_atomic() const
		{
			return (std::reverse_iterator<Function_variable_iterator>(
				new Function_variable_iterator_representation_atomic_prolate_spheroidal(
				false,Function_variable_prolate_spheroidal_handle(const_cast<
				Function_variable_prolate_spheroidal*>(this)))));
		};
		std::reverse_iterator<Function_variable_iterator> rend_atomic() const
		{
			return (std::reverse_iterator<Function_variable_iterator>(
				new Function_variable_iterator_representation_atomic_prolate_spheroidal(
				true,Function_variable_prolate_spheroidal_handle(const_cast<
				Function_variable_prolate_spheroidal*>(this)))));
		};
		Function_size_type number_differentiable()
		{
			Function_prolate_spheroidal_to_rectangular_cartesian_handle
				function_prolate_spheroidal_to_rectangular_cartesian=
				boost::dynamic_pointer_cast<
				Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
				function());
			Function_size_type result;

			result=0;
			if (function_prolate_spheroidal_to_rectangular_cartesian)
			{
				if (lambda)
				{
					result++;
				}
				if (mu)
				{
					result++;
				}
				if (theta)
				{
					result++;
				}
				if (focus)
				{
					result++;
				}
			}

			return (result);
		};
		bool equality_atomic(const Function_variable_handle& variable) const
		{
			bool result;
			Function_prolate_spheroidal_to_rectangular_cartesian_handle
				function_prolate_spheroidal_to_rectangular_cartesian=
				boost::dynamic_pointer_cast<
				Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
				function());
			Function_variable_prolate_spheroidal_handle variable_prolate_spheroidal;

			result=false;
			if (variable_prolate_spheroidal=boost::dynamic_pointer_cast<
				Function_variable_prolate_spheroidal,Function_variable>(variable))
			{
				result=((function_prolate_spheroidal_to_rectangular_cartesian==
					variable_prolate_spheroidal->function())&&
					(lambda==variable_prolate_spheroidal->lambda)&&
					(mu==variable_prolate_spheroidal->mu)&&
					(theta==variable_prolate_spheroidal->theta)&&
					(focus==variable_prolate_spheroidal->focus));
			}

			return (result);
		};
	private:
		bool is_atomic()
		{
			bool result;

			result=false;
			if (this&&((lambda&&!mu&&!theta&&!focus)||(!lambda&&mu&&!theta&&!focus)||
				(!lambda&&!mu&&theta&&!focus)||(!lambda&&!mu&&!theta&&focus)))
			{
				result=true;
			}

			return (result);
		};
	private:
		// copy constructor
		Function_variable_prolate_spheroidal(
			const Function_variable_prolate_spheroidal& variable):
			Function_variable(variable),focus(variable.focus),lambda(variable.lambda),
			mu(variable.mu),theta(variable.theta){}
		// assignment
		Function_variable_prolate_spheroidal& operator=(
			const Function_variable_prolate_spheroidal&);
	private:
		bool focus,lambda,mu,theta;
};

static bool Function_variable_prolate_spheroidal_set_scalar_function(
	Scalar& value,const Function_variable_handle variable)
{
	bool result;
	Function_variable_prolate_spheroidal_handle variable_prolate_spheroidal;
	Function_prolate_spheroidal_to_rectangular_cartesian_handle
		function_prolate_spheroidal_to_rectangular_cartesian;

	result=false;
	if ((variable_prolate_spheroidal=boost::dynamic_pointer_cast<
		Function_variable_prolate_spheroidal,Function_variable>(variable))&&
		(variable_prolate_spheroidal->is_atomic)()&&
		(function_prolate_spheroidal_to_rectangular_cartesian=
		boost::dynamic_pointer_cast<
		Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
		variable_prolate_spheroidal->function())))
	{
		if (variable_prolate_spheroidal->lambda)
		{
			value=
				(function_prolate_spheroidal_to_rectangular_cartesian->lambda_value)();
			result=true;
		}
		else if (variable_prolate_spheroidal->mu)
		{
			value=(function_prolate_spheroidal_to_rectangular_cartesian->mu_value)();
			result=true;
		}
		else if (variable_prolate_spheroidal->theta)
		{
			value=
				(function_prolate_spheroidal_to_rectangular_cartesian->theta_value)();
			result=true;
		}
		else if (variable_prolate_spheroidal->focus)
		{
			value=
				(function_prolate_spheroidal_to_rectangular_cartesian->focus_value)();
			result=true;
		}
	}

	return (result);
}


// class Function_variable_iterator_representation_atomic_prolate_spheroidal
// -------------------------------------------------------------------------

Function_variable_iterator_representation_atomic_prolate_spheroidal::
	Function_variable_iterator_representation_atomic_prolate_spheroidal(
	const bool begin,Function_variable_prolate_spheroidal_handle variable):
	atomic_variable(0),variable(variable)
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
// Constructor.
//==============================================================================
{
	if (begin&&variable)
	{
		if (atomic_variable=boost::dynamic_pointer_cast<
			Function_variable_prolate_spheroidal,Function_variable>(
			variable->clone()))
		{
			if (atomic_variable->lambda)
			{
				atomic_variable->mu=false;
				atomic_variable->theta=false;
				atomic_variable->focus=false;
			}
			else if (atomic_variable->mu)
			{
				atomic_variable->theta=false;
				atomic_variable->focus=false;
			}
			else if (atomic_variable->theta)
			{
				atomic_variable->focus=false;
			}
			atomic_variable->value_private=Function_variable_value_handle(
				new Function_variable_value_scalar(
				Function_variable_prolate_spheroidal_set_scalar_function
				));
		}
	}
}

Function_variable_iterator_representation
	*Function_variable_iterator_representation_atomic_prolate_spheroidal::clone()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	Function_variable_iterator_representation *result;

	result=0;
	if (this)
	{
		result=new
			Function_variable_iterator_representation_atomic_prolate_spheroidal(
			*this);
	}

	return (result);
}

Function_variable_iterator_representation_atomic_prolate_spheroidal::
	~Function_variable_iterator_representation_atomic_prolate_spheroidal()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
// Destructor.
//==============================================================================
{
	// do nothing
}

void Function_variable_iterator_representation_atomic_prolate_spheroidal::
	increment()
//******************************************************************************
// LAST MODIFIED : 30 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	if (variable&&(variable->function())&&(atomic_variable))
	{
		if (atomic_variable->lambda)
		{
			atomic_variable->lambda=false;
			if (variable->mu)
			{
				atomic_variable->mu=true;
			}
			else if (variable->theta)
			{
				atomic_variable->theta=true;
			}
			else if (variable->focus)
			{
				atomic_variable->focus=true;
			}
			else
			{
				atomic_variable=0;
			}
		}
		else if (atomic_variable->mu)
		{
			atomic_variable->mu=false;
			if (variable->theta)
			{
				atomic_variable->theta=true;
			}
			else if (variable->focus)
			{
				atomic_variable->focus=true;
			}
			else
			{
				atomic_variable=0;
			}
		}
		else if (atomic_variable->theta)
		{
			atomic_variable->theta=false;
			if (variable->focus)
			{
				atomic_variable->focus=true;
			}
			else
			{
				atomic_variable=0;
			}
		}
		else
		{
			atomic_variable=0;
		}
	}
}

void Function_variable_iterator_representation_atomic_prolate_spheroidal::
	decrement()
//******************************************************************************
// LAST MODIFIED : 30 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	if (variable&&(variable->function()))
	{
		if (atomic_variable)
		{
			if (atomic_variable->focus)
			{
				atomic_variable->focus=false;
				if (variable->theta)
				{
					atomic_variable->theta=true;
				}
				else if (variable->mu)
				{
					atomic_variable->mu=true;
				}
				else if (variable->lambda)
				{
					atomic_variable->lambda=true;
				}
				else
				{
					atomic_variable=0;
				}
			}
			else if (atomic_variable->theta)
			{
				atomic_variable->theta=false;
				if (variable->mu)
				{
					atomic_variable->mu=true;
				}
				else if (variable->lambda)
				{
					atomic_variable->lambda=true;
				}
				else
				{
					atomic_variable=0;
				}
			}
			else if (atomic_variable->mu)
			{
				atomic_variable->mu=false;
				if (variable->lambda)
				{
					atomic_variable->lambda=true;
				}
				else
				{
					atomic_variable=0;
				}
			}
			else
			{
				atomic_variable=0;
			}
		}
		else
		{
			if (atomic_variable=boost::dynamic_pointer_cast<
				Function_variable_prolate_spheroidal,Function_variable>(
				variable->clone()))
			{
				if (atomic_variable->lambda)
				{
					atomic_variable->mu=false;
					atomic_variable->theta=false;
					atomic_variable->focus=false;
				}
				else if (atomic_variable->mu)
				{
					atomic_variable->theta=false;
					atomic_variable->focus=false;
				}
				else if (atomic_variable->theta)
				{
					atomic_variable->focus=false;
				}
				atomic_variable->value_private=Function_variable_value_handle(
					new Function_variable_value_scalar(
					Function_variable_prolate_spheroidal_set_scalar_function));
			}
		}
	}
}

bool Function_variable_iterator_representation_atomic_prolate_spheroidal::
	equality(const Function_variable_iterator_representation * representation)
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	bool result;
	const Function_variable_iterator_representation_atomic_prolate_spheroidal
		*representation_prolate_spheroidal=dynamic_cast<
		const Function_variable_iterator_representation_atomic_prolate_spheroidal*>(
		representation);

	result=false;
	if (representation_prolate_spheroidal)
	{
		if (((0==atomic_variable)&&
			(0==representation_prolate_spheroidal->atomic_variable))||
			(atomic_variable&&(representation_prolate_spheroidal->atomic_variable)&&
			(*atomic_variable==
			*(representation_prolate_spheroidal->atomic_variable))))
		{
			result=true;
		}
	}

	return (result);
}

Function_variable_handle
	Function_variable_iterator_representation_atomic_prolate_spheroidal::
	dereference() const
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (atomic_variable);
}

Function_variable_iterator_representation_atomic_prolate_spheroidal::
	Function_variable_iterator_representation_atomic_prolate_spheroidal(const
	Function_variable_iterator_representation_atomic_prolate_spheroidal&
	representation):Function_variable_iterator_representation(),
	atomic_variable(0),variable(representation.variable)
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
// Copy constructor.
//==============================================================================
{
	if (representation.atomic_variable)
	{
		atomic_variable=boost::dynamic_pointer_cast<
			Function_variable_prolate_spheroidal,Function_variable>(
			(representation.atomic_variable)->clone());
	}
}
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)

// class Function_variable_matrix_prolate_spheroidal
// -------------------------------------------------

// forward declaration so that can use _handle
class Function_variable_matrix_prolate_spheroidal;
typedef boost::intrusive_ptr<Function_variable_matrix_prolate_spheroidal>
	Function_variable_matrix_prolate_spheroidal_handle;

class Function_variable_matrix_prolate_spheroidal :
	public Function_variable_matrix<Scalar>
//******************************************************************************
// LAST MODIFIED : 13 July 2004
//
// DESCRIPTION :
// component  number
// lambda     1
// mu         2
// theta      3
//==============================================================================
{
	friend class Function_prolate_spheroidal_to_rectangular_cartesian;
	public:
		// constructor
		Function_variable_matrix_prolate_spheroidal(
			const Function_prolate_spheroidal_to_rectangular_cartesian_handle&
			function_prolate_spheroidal_to_rectangular_cartesian,
			Function_size_type component_number):Function_variable_matrix<Scalar>(
			function_prolate_spheroidal_to_rectangular_cartesian,component_number,1)
		{};
		Function_variable_matrix_prolate_spheroidal(
			const Function_prolate_spheroidal_to_rectangular_cartesian_handle&
			function_prolate_spheroidal_to_rectangular_cartesian,
			const std::string component_name):Function_variable_matrix<Scalar>(
			function_prolate_spheroidal_to_rectangular_cartesian,0,1)
		{
			if (function_prolate_spheroidal_to_rectangular_cartesian)
			{
				Function_size_type local_component_number;

				local_component_number=0;
				if ((std::string("lambda")==component_name)||
					(std::string("LAMBDA")==component_name))
				{
					local_component_number=1;
				}
				else if ((std::string("mu")==component_name)||
					(std::string("MU")==component_name))
				{
					local_component_number=2;
				}
				else if ((std::string("theta")==component_name)||
					(std::string("THETA")==component_name))
				{
					local_component_number=3;
				}
				if (0<local_component_number)
				{
					row=local_component_number;
					value_private=Function_variable_value_handle(
						new Function_variable_value_specific<Scalar>(
						Function_variable_matrix_set_value_function<Scalar>));
				}
			}
		};
		~Function_variable_matrix_prolate_spheroidal(){};
	// inherited
	public:
		Function_variable_handle clone() const
		{
			return (Function_variable_handle(
				new Function_variable_matrix_prolate_spheroidal(*this)));
		};
		string_handle get_string_representation()
		{
			string_handle return_string(0);

			if (return_string=new std::string)
			{
				std::ostringstream out;

				switch (row)
				{
					case 0:
					{
						out << "(lambda,mu,theta)";
					} break;
					case 1:
					{
						out << "lambda";
					} break;
					case 2:
					{
						out << "mu";
					} break;
					case 3:
					{
						out << "theta";
					} break;
				}
				*return_string=out.str();
			}

			return (return_string);
		};
		Function_variable_matrix_handle operator()(
			Function_size_type row,Function_size_type column)
		{
			Function_variable_matrix_handle result(0);

			if ((0<row)&&(1==column))
			{
				Function_prolate_spheroidal_to_rectangular_cartesian_handle
					function_prolate_spheroidal_to_rectangular_cartesian=
					boost::dynamic_pointer_cast<
					Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
					function());

				result=Function_variable_matrix_handle(
					new Function_variable_matrix_prolate_spheroidal(
					function_prolate_spheroidal_to_rectangular_cartesian,row));
			}

			return (result);
		};
		Function_size_type number_of_rows() const
		{
			return (3);
		};
		Function_size_type number_of_columns() const
		{
			return (1);
		};
		bool get_entry(Scalar& value) const
		{
			bool result;
			Function_prolate_spheroidal_to_rectangular_cartesian_handle
				function_prolate_spheroidal_to_rectangular_cartesian;

			result=false;
			if ((0<row)&&(1==column)&&
				(function_prolate_spheroidal_to_rectangular_cartesian=
				boost::dynamic_pointer_cast<
				Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
				function())))
			{
				switch (row)
				{
					case 1:
					{
						value=(function_prolate_spheroidal_to_rectangular_cartesian->
							lambda_value)();
						result=true;
					} break;
					case 2:
					{
						value=(function_prolate_spheroidal_to_rectangular_cartesian->
							mu_value)();
						result=true;
					} break;
					case 3:
					{
						value=(function_prolate_spheroidal_to_rectangular_cartesian->
							theta_value)();
						result=true;
					} break;
				}
			}

			return (result);
		};
	private:
		// copy constructor
		Function_variable_matrix_prolate_spheroidal(
			const Function_variable_matrix_prolate_spheroidal& variable):
			Function_variable_matrix<Scalar>(variable){};
};


// class Function_variable_matrix_focus
// ------------------------------------

// forward declaration so that can use _handle
class Function_variable_matrix_focus;
typedef boost::intrusive_ptr<Function_variable_matrix_focus>
	Function_variable_matrix_focus_handle;

class Function_variable_matrix_focus : public Function_variable_matrix<Scalar>
//******************************************************************************
// LAST MODIFIED : 13 July 2004
//
// DESCRIPTION :
//==============================================================================
{
	friend class Function_prolate_spheroidal_to_rectangular_cartesian;
	public:
		// constructor
		Function_variable_matrix_focus(
			const Function_prolate_spheroidal_to_rectangular_cartesian_handle&
			function_prolate_spheroidal_to_rectangular_cartesian):
			Function_variable_matrix<Scalar>(
			function_prolate_spheroidal_to_rectangular_cartesian,1,1){};
		~Function_variable_matrix_focus(){};
	// inherited
	public:
		Function_variable_handle clone() const
		{
			return (Function_variable_handle(
				new Function_variable_matrix_focus(*this)));
		};
		string_handle get_string_representation()
		{
			string_handle return_string(0);

			if (return_string=new std::string)
			{
				std::ostringstream out;

				out << "focus";
				*return_string=out.str();
			}

			return (return_string);
		};
		Function_variable_matrix_handle operator()(
			Function_size_type row,Function_size_type column)
		{
			Function_variable_matrix_handle result(0);

			if ((1==row)&&(1==column))
			{
				Function_prolate_spheroidal_to_rectangular_cartesian_handle
					function_prolate_spheroidal_to_rectangular_cartesian=
					boost::dynamic_pointer_cast<
					Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
					function());

				result=Function_variable_matrix_handle(
					new Function_variable_matrix_focus(
					function_prolate_spheroidal_to_rectangular_cartesian));
			}

			return (result);
		};
		Function_size_type number_of_rows() const
		{
			return (1);
		};
		Function_size_type number_of_columns() const
		{
			return (1);
		};
		bool get_entry(Scalar& value) const
		{
			bool result;
			Function_prolate_spheroidal_to_rectangular_cartesian_handle
				function_prolate_spheroidal_to_rectangular_cartesian;

			result=false;
			if ((1==row)&&(1==column)&&
				(function_prolate_spheroidal_to_rectangular_cartesian=
				boost::dynamic_pointer_cast<
				Function_prolate_spheroidal_to_rectangular_cartesian,Function>(
				function())))
			{
				value=(function_prolate_spheroidal_to_rectangular_cartesian->
					focus_value)();
				result=true;
			}

			return (result);
		};
	private:
		// copy constructor
		Function_variable_matrix_focus(
			const Function_variable_matrix_focus& variable):
			Function_variable_matrix<Scalar>(variable){};
};
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)


// global classes
// ==============

// class Function_prolate_spheroidal_to_rectangular_cartesian
// ----------------------------------------------------------

Function_prolate_spheroidal_to_rectangular_cartesian::
	Function_prolate_spheroidal_to_rectangular_cartesian(const Scalar lambda,
	const Scalar mu,const Scalar theta,const Scalar focus):Function(),
	number_of_components_private(3),focus_private(focus),lambda_private(lambda),
	mu_private(mu),theta_private(theta),x_private(0),y_private(0),z_private(0){}
//******************************************************************************
// LAST MODIFIED : 22 June 2004
//
// DESCRIPTION :
// Constructor.
//==============================================================================

Function_prolate_spheroidal_to_rectangular_cartesian::
	~Function_prolate_spheroidal_to_rectangular_cartesian()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
// Destructor.
//==============================================================================
{
	// do nothing
}

string_handle Function_prolate_spheroidal_to_rectangular_cartesian::
	get_string_representation()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	string_handle return_string;
	std::ostringstream out;

	if (return_string=new std::string)
	{
		out << "prolate spheroidal to rectangular cartesian";
		*return_string=out.str();
	}

	return (return_string);
}

Function_variable_handle Function_prolate_spheroidal_to_rectangular_cartesian::
	input()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (Function_variable_handle(new
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),true,true,
		true,true)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_composite(Function_variable_handle(new
		Function_variable_matrix_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),0)),
		Function_variable_handle(new Function_variable_matrix_focus(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this))))
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		));
}

Function_variable_handle Function_prolate_spheroidal_to_rectangular_cartesian::
	output()
//******************************************************************************
// LAST MODIFIED : 4 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (Function_variable_handle(new
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_rectangular_cartesian
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_rectangular_cartesian
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(Function_prolate_spheroidal_to_rectangular_cartesian_handle(this))));
}

Function_variable_handle Function_prolate_spheroidal_to_rectangular_cartesian::
	component(std::string component_name)
//******************************************************************************
// LAST MODIFIED : 4 June 2004
//
// DESCRIPTION :
// Returns the component output.
//==============================================================================
{
	return (Function_variable_handle(new
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_rectangular_cartesian
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_rectangular_cartesian
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),
		component_name)));
}

Function_variable_handle Function_prolate_spheroidal_to_rectangular_cartesian::
	component(Function_size_type component_number)
//******************************************************************************
// LAST MODIFIED : 4 June 2004
//
// DESCRIPTION :
// Returns the component output.
//==============================================================================
{
	return (Function_variable_handle(new
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_rectangular_cartesian
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_rectangular_cartesian
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),
		component_number)));
}

Function_variable_handle Function_prolate_spheroidal_to_rectangular_cartesian::
	focus()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (Function_variable_handle(new
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),false,
		false,false,true)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_focus(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this))
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		));
}

Function_variable_handle Function_prolate_spheroidal_to_rectangular_cartesian::
	lambda()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (Function_variable_handle(new
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),true,
		false,false,false)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),1)
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		));
}

Function_variable_handle Function_prolate_spheroidal_to_rectangular_cartesian::
	mu()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (Function_variable_handle(new
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),false,
		true,false,false)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),2)
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		));
}

Function_variable_handle Function_prolate_spheroidal_to_rectangular_cartesian::
	prolate()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (Function_variable_handle(new
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),true,
		true,true,false)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),0)
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		));
}

Function_variable_handle Function_prolate_spheroidal_to_rectangular_cartesian::
	theta()
//******************************************************************************
// LAST MODIFIED : 3 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (Function_variable_handle(new
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),false,
		false,true,false)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_prolate_spheroidal(
		Function_prolate_spheroidal_to_rectangular_cartesian_handle(this),3)
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		));
}

Scalar Function_prolate_spheroidal_to_rectangular_cartesian::focus_value()
//******************************************************************************
// LAST MODIFIED : 4 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (focus_private);
}

Scalar Function_prolate_spheroidal_to_rectangular_cartesian::lambda_value()
//******************************************************************************
// LAST MODIFIED : 4 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (lambda_private);
}

Scalar Function_prolate_spheroidal_to_rectangular_cartesian::mu_value()
//******************************************************************************
// LAST MODIFIED : 4 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (mu_private);
}

Scalar Function_prolate_spheroidal_to_rectangular_cartesian::theta_value()
//******************************************************************************
// LAST MODIFIED : 4 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (theta_private);
}

Scalar Function_prolate_spheroidal_to_rectangular_cartesian::x_value()
//******************************************************************************
// LAST MODIFIED : 23 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (x_private);
}

Scalar Function_prolate_spheroidal_to_rectangular_cartesian::y_value()
//******************************************************************************
// LAST MODIFIED : 23 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (y_private);
}

Scalar Function_prolate_spheroidal_to_rectangular_cartesian::z_value()
//******************************************************************************
// LAST MODIFIED : 23 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (z_private);
}

Function_size_type Function_prolate_spheroidal_to_rectangular_cartesian::
	number_of_components()
//******************************************************************************
// LAST MODIFIED : 4 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	return (number_of_components_private);
}

Function_handle Function_prolate_spheroidal_to_rectangular_cartesian::evaluate(
	Function_variable_handle atomic_variable)
//******************************************************************************
// LAST MODIFIED : 30 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	Function_handle result(0);
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_rectangular_cartesian_handle
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_matrix_rectangular_cartesian_handle
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		atomic_variable_rectangular_cartesian;

	if ((atomic_variable_rectangular_cartesian=boost::dynamic_pointer_cast<
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_rectangular_cartesian
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_rectangular_cartesian
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		,Function_variable>(
		atomic_variable))&&(Function_handle(this)==
		atomic_variable_rectangular_cartesian->function())&&
		(0<atomic_variable_rectangular_cartesian->
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		component_number
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		row
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		)&&
		(atomic_variable_rectangular_cartesian->
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		component_number
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		row
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		<=number_of_components()))
	{
		Matrix result_matrix(1,1);

		result_matrix(0,0)=0;
		switch (atomic_variable_rectangular_cartesian->
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			component_number
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			row
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			)
		{
			case 1:
			{
				result_matrix(0,0)=(Scalar)((double)focus_private*
					cosh((double)lambda_private)*cos((double)mu_private));
			} break;
			case 2:
			{
				result_matrix(0,0)=(Scalar)((double)focus_private*
					sinh((double)lambda_private)*sin((double)mu_private)*
					cos((double)theta_private));
			} break;
			case 3:
			{
				result_matrix(0,0)=(Scalar)((double)focus_private*
					sinh((double)lambda_private)*sin((double)mu_private)*
					sin((double)theta_private));
			} break;
		}
		result=Function_handle(new Function_matrix(result_matrix));
	}
	else
	{
		result=get_value(atomic_variable);
	}

	return (result);
}

bool Function_prolate_spheroidal_to_rectangular_cartesian::evaluate_derivative(
	Scalar& derivative,Function_variable_handle atomic_variable,
	std::list<Function_variable_handle>& atomic_independent_variables)
//******************************************************************************
// LAST MODIFIED : 30 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	bool result;
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_matrix_focus_handle atomic_variable_focus;
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_prolate_spheroidal_handle
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_matrix_prolate_spheroidal_handle
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		atomic_variable_prolate_spheroidal;
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_rectangular_cartesian_handle
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_matrix_rectangular_cartesian_handle
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		atomic_variable_rectangular_cartesian;

	result=false;
	if ((atomic_variable_prolate_spheroidal=boost::dynamic_pointer_cast<
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_prolate_spheroidal
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		,Function_variable>(atomic_variable))&&
		(Function_handle(this)==atomic_variable_prolate_spheroidal->function())&&
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(atomic_variable_prolate_spheroidal->is_atomic)()&&
		(1==atomic_variable_prolate_spheroidal->number_differentiable())
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(0<atomic_variable_prolate_spheroidal->row)
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		)
	{
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal_handle
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_prolate_spheroidal_handle
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			atomic_independent_variable;

		result=true;
		if ((1==atomic_independent_variables.size())&&
			(atomic_independent_variable=boost::dynamic_pointer_cast<
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			Function_variable_prolate_spheroidal
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			Function_variable_matrix_prolate_spheroidal
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			,Function_variable>(
			atomic_independent_variables.front()))&&
			(*atomic_variable_prolate_spheroidal== *atomic_independent_variable))
		{
			derivative=1;
		}
		else
		{
			derivative=0;
		}
	}
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	else if ((atomic_variable_focus=boost::dynamic_pointer_cast<
		Function_variable_matrix_focus,Function_variable>(atomic_variable))&&
		(Function_handle(this)==atomic_variable_focus->function()))
	{
		Function_variable_matrix_focus_handle atomic_independent_variable;

		result=true;
		if ((1==atomic_independent_variables.size())&&
			(atomic_independent_variable=boost::dynamic_pointer_cast<
			Function_variable_matrix_focus,Function_variable>(
			atomic_independent_variables.front()))&&
			(*atomic_variable_focus== *atomic_independent_variable))
		{
			derivative=1;
		}
		else
		{
			derivative=0;
		}
	}
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	else if ((atomic_variable_rectangular_cartesian=boost::dynamic_pointer_cast<
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_rectangular_cartesian
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_rectangular_cartesian
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		,Function_variable>(atomic_variable))&&
		(Function_handle(this)==atomic_variable_rectangular_cartesian->function())&&
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(atomic_variable_rectangular_cartesian->is_atomic)()&&
		(1==atomic_variable_rectangular_cartesian->number_differentiable())
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(0<atomic_variable_rectangular_cartesian->row)
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		)
	{
		bool zero_derivative;
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_focus_handle variable_focus;
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal_handle
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_prolate_spheroidal_handle
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			variable_prolate_spheroidal;
		Function_size_type focus_derivative_order,i,lambda_derivative_order,
			mu_derivative_order,theta_derivative_order;
		std::list<Function_variable_handle>::iterator independent_variable_iterator;

		// check for zero derivative and calculate derivative orders
		zero_derivative=false;
		lambda_derivative_order=0;
		mu_derivative_order=0;
		theta_derivative_order=0;
		focus_derivative_order=0;
		independent_variable_iterator=atomic_independent_variables.begin();
		i=atomic_independent_variables.size();
		while (!zero_derivative&&(i>0))
		{
			if ((variable_prolate_spheroidal=boost::dynamic_pointer_cast<
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
				Function_variable_prolate_spheroidal
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
				Function_variable_matrix_prolate_spheroidal
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
				,Function_variable>(*independent_variable_iterator))&&
				(variable_prolate_spheroidal->function()==Function_handle(this)))
			{
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
				if (variable_prolate_spheroidal->lambda)
				{
					lambda_derivative_order++;
				}
				else if (variable_prolate_spheroidal->mu)
				{
					mu_derivative_order++;
				}
				else if (variable_prolate_spheroidal->theta)
				{
					theta_derivative_order++;
				}
				else if (variable_prolate_spheroidal->focus)
				{
					focus_derivative_order++;
					if (1<focus_derivative_order)
					{
						zero_derivative=true;
					}
				}
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
				switch (variable_prolate_spheroidal->row)
				{
					case 1:
					{
						lambda_derivative_order++;
					} break;
					case 2:
					{
						mu_derivative_order++;
					} break;
					case 3:
					{
						theta_derivative_order++;
					} break;
					default:
					{
						zero_derivative=true;
					} break;
				}
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			}
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			else if ((variable_focus=boost::dynamic_pointer_cast<
				Function_variable_matrix_focus,Function_variable>(
				*independent_variable_iterator))&&
				(variable_focus->function()==Function_handle(this)))
			{
				focus_derivative_order++;
				if (1<focus_derivative_order)
				{
					zero_derivative=true;
				}
			}
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			else
			{
				zero_derivative=true;
			}
			independent_variable_iterator++;
			i--;
		}
		if (zero_derivative)
		{
			result=true;
			derivative=0;
		}
		else
		{
			Scalar focus_derivative;

			// focus_derivative_order<=1 from construction above
			if (0==focus_derivative_order)
			{
				focus_derivative=focus_private;
			}
			else
			{
				focus_derivative=1;
			}
			// assign derivatives
			switch (atomic_variable_rectangular_cartesian->
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
				component_number
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
				row
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
				)
			{
				case 1:
				{
					switch (lambda_derivative_order)
					{
						case 0:
						{
							switch (mu_derivative_order)
							{
								case 0:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												cosh(lambda_private)*cos(mu_private);
										} break;
										case 1:
										{
											result=true;
											derivative=0;
										} break;
										case 2:
										{
											result=true;
											derivative=0;
										} break;
									}
								} break;
								case 1:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative= -focus_derivative*
												cosh(lambda_private)*sin(mu_private);
										} break;
										case 1:
										{
											result=true;
											derivative=0;
										} break;
									}
								} break;
								case 2:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative= -focus_derivative*
												cosh(lambda_private)*cos(mu_private);
										} break;
									}
								} break;
							}
						} break;
						case 1:
						{
							switch (mu_derivative_order)
							{
								case 0:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												sinh(lambda_private)*cos(mu_private);
										} break;
										case 1:
										{
											result=true;
											derivative=0;
										} break;
									}
								} break;
								case 1:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative= -focus_derivative*
												sinh(lambda_private)*sin(mu_private);
										} break;
									}
								} break;
							}
						} break;
						case 2:
						{
							// mu order
							switch (mu_derivative_order)
							{
								case 0:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												cosh(lambda_private)*cos(mu_private);
										} break;
									}
								} break;
							}
						} break;
					}
				} break;
				case 2:
				{
					switch (lambda_derivative_order)
					{
						case 0:
						{
							switch (mu_derivative_order)
							{
								case 0:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												sinh(lambda_private)*sin(mu_private)*cos(theta_private);
										} break;
										case 1:
										{
											result=true;
											derivative= -focus_derivative*
												sinh(lambda_private)*sin(mu_private)*sin(theta_private);
										} break;
										case 2:
										{
											result=true;
											derivative= -focus_derivative*
												sinh(lambda_private)*sin(mu_private)*cos(theta_private);
										} break;
									}
								} break;
								case 1:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*sinh(lambda_private)*
												cos(mu_private)*cos(theta_private);
										} break;
										case 1:
										{
											result=true;
											derivative= -focus_derivative*sinh(lambda_private)*
												cos(mu_private)*sin(theta_private);
										} break;
									}
								} break;
								case 2:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative= -focus_derivative*sinh(lambda_private)*
												sin(mu_private)*cos(theta_private);
										} break;
									}
								} break;
							}
						} break;
						case 1:
						{
							switch (mu_derivative_order)
							{
								case 0:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												cosh(lambda_private)*sin(mu_private)*cos(theta_private);
										} break;
										case 1:
										{
											result=true;
											derivative= -focus_derivative*
												cosh(lambda_private)*sin(mu_private)*sin(theta_private);
										} break;
									}
								} break;
								case 1:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												cosh(lambda_private)*cos(mu_private)*cos(theta_private);
										} break;
									}
								} break;
							}
						} break;
						case 2:
						{
							// mu order
							switch (mu_derivative_order)
							{
								case 0:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												sinh(lambda_private)*sin(mu_private)*cos(theta_private);
										} break;
									}
								} break;
							}
						} break;
					}
				} break;
				case 3:
				{
					switch (lambda_derivative_order)
					{
						case 0:
						{
							switch (mu_derivative_order)
							{
								case 0:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												sinh(lambda_private)*sin(mu_private)*sin(theta_private);
										} break;
										case 1:
										{
											result=true;
											derivative=focus_derivative*
												sinh(lambda_private)*sin(mu_private)*cos(theta_private);
										} break;
										case 2:
										{
											result=true;
											derivative= -focus_derivative*
												sinh(lambda_private)*sin(mu_private)*sin(theta_private);
										} break;
									}
								} break;
								case 1:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												sinh(lambda_private)*cos(mu_private)*sin(theta_private);
										} break;
										case 1:
										{
											result=true;
											derivative=focus_derivative*
												sinh(lambda_private)*cos(mu_private)*cos(theta_private);
										} break;
									}
								} break;
								case 2:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative= -focus_derivative*
												sinh(lambda_private)*sin(mu_private)*sin(theta_private);
										} break;
									}
								} break;
							}
						} break;
						case 1:
						{
							switch (mu_derivative_order)
							{
								case 0:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												cosh(lambda_private)*sin(mu_private)*sin(theta_private);
										} break;
										case 1:
										{
											result=true;
											derivative=focus_derivative*
												cosh(lambda_private)*sin(mu_private)*cos(theta_private);
										} break;
									}
								} break;
								case 1:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												cosh(lambda_private)*cos(mu_private)*sin(theta_private);
										} break;
									}
								} break;
							}
						} break;
						case 2:
						{
							// mu order
							switch (mu_derivative_order)
							{
								case 0:
								{
									switch (theta_derivative_order)
									{
										case 0:
										{
											result=true;
											derivative=focus_derivative*
												sinh(lambda_private)*sin(mu_private)*sin(theta_private);
										} break;
									}
								} break;
							}
						} break;
					}
				} break;
				default:
				{
					result=true;
					derivative=0;
				} break;
			}
		}
	}

	return (result);
}

bool Function_prolate_spheroidal_to_rectangular_cartesian::set_value(
	Function_variable_handle atomic_variable,
	Function_variable_handle atomic_value)
//******************************************************************************
// LAST MODIFIED : 30 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	bool result;
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_matrix_focus_handle atomic_focus_variable;
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_prolate_spheroidal_handle
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_matrix_prolate_spheroidal_handle
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		atomic_prolate_spheroidal_variable;
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_rectangular_cartesian_handle
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_matrix_rectangular_cartesian_handle
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		atomic_rectangular_cartesian_variable;
	Function_variable_value_scalar_handle value_scalar;

	result=false;
	if ((atomic_prolate_spheroidal_variable=
		boost::dynamic_pointer_cast<
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_prolate_spheroidal
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		,Function_variable>(atomic_variable))&&
		(Function_handle(this)==atomic_prolate_spheroidal_variable->function())&&
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(atomic_prolate_spheroidal_variable->is_atomic)()&&
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(0<atomic_prolate_spheroidal_variable->row)&&
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		atomic_value&&(atomic_value->value())&&
		(std::string("Scalar")==(atomic_value->value())->type())&&
		(value_scalar=boost::dynamic_pointer_cast<Function_variable_value_scalar,
		Function_variable_value>(atomic_value->value())))
	{
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		if (atomic_prolate_spheroidal_variable->lambda)
		{
			result=value_scalar->set(lambda_private,atomic_value);
		}
		else if (atomic_prolate_spheroidal_variable->mu)
		{
			result=value_scalar->set(mu_private,atomic_value);
		}
		else if (atomic_prolate_spheroidal_variable->theta)
		{
			result=value_scalar->set(theta_private,atomic_value);
		}
		else if (atomic_prolate_spheroidal_variable->focus)
		{
			result=value_scalar->set(focus_private,atomic_value);
		}
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		switch (atomic_prolate_spheroidal_variable->row)
		{
			case 1:
			{
				result=value_scalar->set(lambda_private,atomic_value);
			} break;
			case 2:
			{
				result=value_scalar->set(mu_private,atomic_value);
			} break;
			case 3:
			{
				result=value_scalar->set(theta_private,atomic_value);
			} break;
		}
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	}
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	else if ((atomic_focus_variable=boost::dynamic_pointer_cast<
		Function_variable_matrix_focus,Function_variable>(atomic_variable))&&
		(Function_handle(this)==atomic_focus_variable->function())&&
		atomic_value&&(atomic_value->value())&&
		(std::string("Scalar")==(atomic_value->value())->type())&&
		(value_scalar=boost::dynamic_pointer_cast<Function_variable_value_scalar,
		Function_variable_value>(atomic_value->value())))
	{
		result=value_scalar->set(focus_private,atomic_value);
	}
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	else if ((atomic_rectangular_cartesian_variable=
		boost::dynamic_pointer_cast<
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_rectangular_cartesian
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_rectangular_cartesian
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		,Function_variable>(atomic_variable))&&
		(Function_handle(this)==atomic_rectangular_cartesian_variable->function())&&
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(atomic_rectangular_cartesian_variable->is_atomic)()&&
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		(0<atomic_rectangular_cartesian_variable->row)&&
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		atomic_value&&(atomic_value->value())&&
		(std::string("Scalar")==(atomic_value->value())->type())&&
		(value_scalar=boost::dynamic_pointer_cast<Function_variable_value_scalar,
		Function_variable_value>(atomic_value->value())))
	{
		switch (atomic_rectangular_cartesian_variable->
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			component_number
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			row
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			)
		{
			case 1:
			{
				result=value_scalar->set(x_private,atomic_value);
			} break;
			case 2:
			{
				result=value_scalar->set(y_private,atomic_value);
			} break;
			case 3:
			{
				result=value_scalar->set(z_private,atomic_value);
			} break;
		}
	}

	return (result);
}

Function_handle Function_prolate_spheroidal_to_rectangular_cartesian::get_value(
	Function_variable_handle atomic_variable)
//******************************************************************************
// LAST MODIFIED : 30 June 2004
//
// DESCRIPTION :
//==============================================================================
{
	Function_handle result(0);
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_matrix_focus_handle atomic_variable_focus;
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_prolate_spheroidal_handle
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_matrix_prolate_spheroidal_handle
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		atomic_variable_prolate_spheroidal;
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_rectangular_cartesian_handle
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	Function_variable_matrix_rectangular_cartesian_handle
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		atomic_variable_rectangular_cartesian;

	if ((atomic_variable_prolate_spheroidal=boost::dynamic_pointer_cast<
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_prolate_spheroidal
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_prolate_spheroidal
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		,Function_variable>(atomic_variable))&&
		(Function_handle(this)==atomic_variable_prolate_spheroidal->function()))
	{
		Matrix result_matrix(1,1);

		if (
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			Function_variable_prolate_spheroidal_set_scalar_function(
			result_matrix(0,0),atomic_variable)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			atomic_variable_prolate_spheroidal->get_entry(result_matrix(0,0))
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			)
		{
			result=Function_handle(new Function_matrix(result_matrix));
		}
	}
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	if ((atomic_variable_focus=boost::dynamic_pointer_cast<
		Function_variable_matrix_focus,Function_variable>(atomic_variable))&&
		(Function_handle(this)==atomic_variable_focus->function()))
	{
		Matrix result_matrix(1,1);

		if (atomic_variable_focus->get_entry(result_matrix(0,0)))
		{
			result=Function_handle(new Function_matrix(result_matrix));
		}
	}
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
	else if ((atomic_variable_rectangular_cartesian=boost::dynamic_pointer_cast<
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_rectangular_cartesian
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		Function_variable_matrix_rectangular_cartesian
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
		,Function_variable>(
		atomic_variable))&&(Function_handle(this)==
		atomic_variable_rectangular_cartesian->function()))
	{
		Matrix result_matrix(1,1);

		if (
#if defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			Function_variable_rectangular_cartesian_set_scalar_function(
			result_matrix(0,0),atomic_variable)
#else // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			atomic_variable_rectangular_cartesian->get_entry(result_matrix(0,0))
#endif // defined (BEFORE_FUNCTION_VARIABLE_MATRIX_ABSTRACT)
			)
		{
			result=Function_handle(new Function_matrix(result_matrix));
		}
	}

	return (result);
}

Function_prolate_spheroidal_to_rectangular_cartesian::
	Function_prolate_spheroidal_to_rectangular_cartesian(
	const Function_prolate_spheroidal_to_rectangular_cartesian& function):
	Function(),
	number_of_components_private(function.number_of_components_private),
	focus_private(function.focus_private),lambda_private(function.lambda_private),
	mu_private(function.mu_private),theta_private(function.theta_private),
	x_private(function.x_private),y_private(function.y_private),
	z_private(function.z_private){}
//******************************************************************************
// LAST MODIFIED : 23 June 2004
//
// DESCRIPTION :
// Copy constructor.
//==============================================================================

Function_prolate_spheroidal_to_rectangular_cartesian&
	Function_prolate_spheroidal_to_rectangular_cartesian::operator=(
	const Function_prolate_spheroidal_to_rectangular_cartesian& function)
//******************************************************************************
// LAST MODIFIED : 22 June 2004
//
// DESCRIPTION :
// Assignment operator.
//==============================================================================
{
	lambda_private=function.lambda_private;
	mu_private=function.mu_private;
	theta_private=function.theta_private;
	focus_private=function.focus_private;
	x_private=function.x_private;
	y_private=function.y_private;
	z_private=function.z_private;

	return (*this);
}
