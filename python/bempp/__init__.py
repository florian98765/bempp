""" Boundary Element Method package BEM++ """

__all__=['GmshInterface',
         'grid_from_sphere',
         'grid_from_element_data',
         'structured_grid',
         'function_space',
         'GridFunction',
         'global_parameters',
         'BlockedBoundaryOperator',
         'export']

from bempp.grid import Grid,grid_from_sphere,grid_from_element_data, structured_grid
from bempp.assembly import GridFunction
from bempp.assembly import BlockedBoundaryOperator
from bempp.space import function_space
from bempp.file_interfaces.gmsh import GmshInterface
from bempp.common import global_parameters
from bempp.file_interfaces import export

def test():
    """ Runs BEM++ python unit tests """
    from py.test import main
    from os.path import dirname
    main(dirname(__file__))
