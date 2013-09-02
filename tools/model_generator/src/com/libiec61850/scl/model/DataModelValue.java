package com.libiec61850.scl.model;

import com.libiec61850.scl.types.EnumerationType;
import com.libiec61850.scl.types.IllegalValueException;
import com.libiec61850.scl.types.SclType;

public class DataModelValue {

    private Object value = null;
    
    public DataModelValue(AttributeType type, SclType sclType, String value) throws IllegalValueException {
        switch (type) {
        case ENUMERATED:
            EnumerationType enumType = (EnumerationType) sclType;
            this.value = (Object) (new Integer(enumType.getOrdByEnumString(value)));
            break;
        case INT8:
        case INT16:
        case INT32:
        case INT8U:
        case INT16U:
        case INT32U:
        case INT24U:
        case INT64:
            this.value = new Long(value);
            break;
        case BOOLEAN:
            if (value.toLowerCase().equals("true"))
                this.value = new Boolean(true);
            else
                this.value = new Boolean(false);
            break;
        case FLOAT32:
        	this.value = new Float(value);
        	break;
        case FLOAT64:
        	this.value = new Double(value);
        case UNICODE_STRING_255:
        	this.value = value;
        	break;
            
        case VISIBLE_STRING_255:
            this.value = (Object) value;
            break;
        default:
            throw new IllegalValueException("Unsupported type " + type.toString());
        }
    }
    
    public Object getValue() {
        return value;
    }
    
    public long getLongValue() {
        return (Long) value;
    }
    
    public int getIntValue() {
    	if (value instanceof Long) {
    		Long longValue = (Long) value;
    		return ((Long) value).intValue();
    	}
    	if (value instanceof Integer)
    		return ((Integer) value).intValue();
    
        if (value instanceof Boolean) {
        	if (((Boolean) value) == true)
        		return 1;
        	else
        		return 0;
        }
        
        return 0;
    }
    
}
