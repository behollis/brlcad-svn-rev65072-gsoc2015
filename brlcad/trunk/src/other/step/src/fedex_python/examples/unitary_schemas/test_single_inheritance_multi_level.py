# This file was generated by fedex_python.  You probably don't want to edit
# it since your modifications will be lost if fedex_plus is used to
# regenerate it.
from SCL.SCLBase import *
from SCL.SimpleDataTypes import *
from SCL.ConstructedDataTypes import *
from SCL.AggregationDataTypes import *
from SCL.TypeChecker import check_type
from SCL.Expr import *
length_measure = REAL
label = STRING
point = REAL

####################
 # ENTITY shape #
####################
class shape(BaseEntityClass):
	'''Entity shape definition.

	:param item_name
	:type item_name:STRING

	:param number_of_sides
	:type number_of_sides:INTEGER
	'''
	def __init__( self , item_name,number_of_sides, ):
		self.item_name = item_name
		self.number_of_sides = number_of_sides

	@apply
	def item_name():
		def fget( self ):
			return self._item_name
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument item_name is mantatory and can not be set to None')
			if not check_type(value,STRING):
				self._item_name = STRING(value)
			else:
				self._item_name = value
		return property(**locals())

	@apply
	def number_of_sides():
		def fget( self ):
			return self._number_of_sides
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument number_of_sides is mantatory and can not be set to None')
			if not check_type(value,INTEGER):
				self._number_of_sides = INTEGER(value)
			else:
				self._number_of_sides = value
		return property(**locals())

####################
 # ENTITY subshape #
####################
class subshape(shape):
	'''Entity subshape definition.
	'''
	def __init__( self , shape__item_name , shape__number_of_sides ,  ):
		shape.__init__(self , shape__item_name , shape__number_of_sides , )

####################
 # ENTITY rectangle #
####################
class rectangle(subshape):
	'''Entity rectangle definition.

	:param height
	:type height:REAL

	:param width
	:type width:REAL
	'''
	def __init__( self , height,width, ):
		subshape.__init__(self , )
		self.height = height
		self.width = width

	@apply
	def height():
		def fget( self ):
			return self._height
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument height is mantatory and can not be set to None')
			if not check_type(value,REAL):
				self._height = REAL(value)
			else:
				self._height = value
		return property(**locals())

	@apply
	def width():
		def fget( self ):
			return self._width
		def fset( self, value ):
		# Mandatory argument
			if value==None:
				raise AssertionError('Argument width is mantatory and can not be set to None')
			if not check_type(value,REAL):
				self._width = REAL(value)
			else:
				self._width = value
		return property(**locals())
