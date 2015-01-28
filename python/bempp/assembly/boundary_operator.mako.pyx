<%
from sys import version
from bempp_operators import dtypes, compatible_dtypes, bops
from data_types import scalar_cython_type
%>
from bempp.space.space cimport Space
from discrete_boundary_operator cimport DiscreteBoundaryOperator
from discrete_boundary_operator cimport DenseDiscreteBoundaryOperator
from discrete_boundary_operator cimport DiscreteBoundaryOperatorBase
from discrete_boundary_operator import SparseDiscreteBoundaryOperator
from bempp.utils.byte_conversion import convert_to_bytes
from bempp.assembly.grid_function cimport GridFunction
from bempp.utils cimport shared_ptr,static_pointer_cast
from bempp.utils import combined_type

from numpy cimport dtype

from bempp.utils cimport complex_float,complex_double
import numpy as np
cimport numpy as np

np.import_array()

cdef class BoundaryOperatorBase:
    """

    A boundary operator is an abstract representation of an operator
    acting on the boundary of a mesh. It can be multiplied with GridFunction
    objects and provides an operation
    `weak_form` that returns a discrete representation of the boundary
    operator.

    Notes
    -----
    Boundary operators are not constructed directly but through factory
    functions (see for example bempp.operators)

    """
    

    def __cinit__(self):
        pass

    def __init__(self):
        pass

    property basis_type:
        def __get__(self):
            return self._basis_type

    property result_type:
        def __get__(self):
            return self._result_type

    def __add__(self,BoundaryOperatorBase other):

       return _SumBoundaryOperator(self,other)

    def __mul__(self, object x):

        if not isinstance(self,BoundaryOperatorBase):
            return x*self

        if np.isscalar(x):
            return _ScaledBoundaryOperator(self,x)

        if isinstance(x,GridFunction):
            return self._apply_grid_function(x)

        raise ValueError("Multiplication not supported for type"+str(type(x)))

    def __neg__(self):

        return _ScaledBoundaryOperator(self,-1.0)

    def __sub__(self,BoundaryOperatorBase other):

        return self.__add__(-other)

    def weak_form(self):
        """

        This method returns an assembled representation
        of a boundary operator that allows matrix-vector
        multiplication.

        Returns
        -------
        discrete_operator : bempp.assembly.DiscreteBoundaryOperatorBase
            A discrete representation of the boundary operator.

        """

        raise NotImplementedError("Method not implemented")

    def _apply_grid_function(self,GridFunction g):
        raise NotImplementedError("Method not implemented")

cdef class GeneralBoundaryOperator(BoundaryOperatorBase):
    """ Holds a reference to a boundary operator """


    def __cinit__(self, basis_type=None, result_type=None):
        pass


    def __init__(self, basis_type=None, result_type=None):
        from numpy import dtype

        self._basis_type = basis_type
        self._result_type = result_type

% for i, (pybasis, cybasis) in enumerate(dtypes.items()):
%     for j, (pyresult, cyresult) in enumerate(dtypes.items()):
%         if pyresult in compatible_dtypes[pybasis]:
        if basis_type == "${pybasis}" and result_type == "${pyresult}":
            self.impl_.set${cybasis}${cyresult}()
%         endif
%     endfor
% endfor

    def _apply_grid_function(self,GridFunction g):
        if not(self.domain.is_compatible(g.space)):
            raise ValueError("Spaces do not match")

        op_w =  self.weak_form()
        coeffs = g.coefficients
        result_projections = (op_w*coeffs)
        return GridFunction(self.range,dual_space=self.dual_to_range,
                projections=result_projections,parameter_list=g.parameter_list_)

    def weak_form(self):
        cdef DiscreteBoundaryOperator dbop = DiscreteBoundaryOperator()
        
% for pybasis,cybasis in dtypes.items():
%     for pyresult,cyresult in dtypes.items():
%         if pyresult in compatible_dtypes[pybasis]:

        if self.basis_type=="${pybasis}" and self.result_type=="${pyresult}":
            dbop._impl_${pyresult}_.assign(_boundary_operator_variant_weak_form[${cybasis},${cyresult}](self.impl_))
            dbop._dtype = self.result_type
            return dbop
%          endif
%      endfor
% endfor
        raise ValueError("Incompatible basis and result types") 
        

% for variable in ['domain', 'range', 'dual_to_range']:
    property ${variable}:
        def __get__(self):
            if not self.impl_.valid_${variable}():
                return None
            cdef Space result = Space.__new__(Space)
            result.impl_ = self.impl_.${variable}()
            return result
% endfor


    property label:
        def __get__(self):
            return self.impl_.label().decode("UTF-8")

cdef class _ScaledBoundaryOperator(BoundaryOperatorBase):
    cdef BoundaryOperatorBase _op
    cdef string _label
    cdef object _alpha

    def __cinit__(self,BoundaryOperatorBase op,object alpha):
        pass

    def __init__(self,BoundaryOperatorBase op, object alpha):
        
        self._alpha = 1.0*alpha # make sure it is not integer
        self._basis_type = op._basis_type
        self._result_type = combined_type(np.dtype(type(self._alpha)),op.result_type)
        self._op = op

    property label:
        def __get__(self):
            return self._label.decode("UTF-8")

    def weak_form(self):
        return self._alpha*self._op.weak_form()

    def _apply_grid_function(self,GridFunction g):

        return self._alpha*(self._op*g)

cdef class _SumBoundaryOperator(BoundaryOperatorBase):
    cdef BoundaryOperatorBase _op1
    cdef BoundaryOperatorBase _op2
    cdef string _label

    def __cinit__(self,BoundaryOperatorBase op1, BoundaryOperatorBase op2):
        pass


    def __init__(self,BoundaryOperatorBase op1, BoundaryOperatorBase op2):

        self._basis_type = combined_type(op1._basis_type,op2._basis_type)
        self._result_type = combined_type(op1._result_type,op2._result_type)

        self._op1 = op1
        self._op2 = op2

        self._label = ("("+op1.label+"+"+op2.label+")").encode("UTF-8")

    property label:

        def __get__(self):
            return self._label.decode("UTF-8")

    def weak_form(self):

        return self._op1.weak_form()+self._op2.weak_form()

    def _apply_grid_function(self, GridFunction g):

        return self._op1*g+self._op2*g



cdef class SparseBoundaryOperator(GeneralBoundaryOperator):
    """ Holds a reference to a boundary operator """


    def __cinit__(self, basis_type=None, result_type=None):
        pass


    def __init__(self, basis_type=None, result_type=None):

        super(SparseBoundaryOperator,self).__init__(basis_type,result_type)

    def weak_form(self):

        import scipy.sparse

        cdef DiscreteBoundaryOperator discrete_operator = super(SparseBoundaryOperator,self).weak_form()
        res = None
% for pyvalue,cyvalue in dtypes.items():

        if discrete_operator.dtype=="${pyvalue}":
            res = py_get_sparse_from_discrete_operator[${cyvalue}](discrete_operator._impl_${pyvalue}_)
% endfor

        if res is None:
            raise ValueError("Unknown data type")

        return SparseDiscreteBoundaryOperator(res)

cdef class DenseBoundaryOperator(GeneralBoundaryOperator):


    def __cinit__(self, GeneralBoundaryOperator op):
        pass


    def __init__(self,GeneralBoundaryOperator op):

        super(DenseBoundaryOperator,self).__init__(basis_type=op.basis_type,result_type=op.result_type)
        self.impl_ = op.impl_

    def weak_form(self):
        cdef DenseDiscreteBoundaryOperator dbop = DenseDiscreteBoundaryOperator()
        
% for pybasis,cybasis in dtypes.items():
%     for pyresult,cyresult in dtypes.items():
%         if pyresult in compatible_dtypes[pybasis]:

        if self.basis_type=="${pybasis}" and self.result_type=="${pyresult}":
            dbop._impl_${pyresult}_.assign(_boundary_operator_variant_weak_form[${cybasis},${cyresult}](self.impl_))
            dbop._dtype = self.result_type
            return dbop
%          endif
%      endfor
% endfor
        raise ValueError("Incompatible basis and result types") 
        
