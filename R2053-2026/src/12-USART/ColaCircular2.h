/*
 * ColaCircular.h
 *
 *  Created on: Nov 10, 2024
 *      Author: gmand
 */



#if CC_MODE_WORK ==2

#ifndef COLACIRCULAR_H_
#define COLACIRCULAR_H_

 enum newLineType_t{CR,LF,CR_LF};


template <typename T>
class ColaCircular {

private:

    unsigned int m_idx1, m_idx2; // indices de insercion y extraccion
    unsigned int m_size;  // tama#o definido por el usurario
    T *queue;
//    int m_cont; // Para llevar el conteo de elementos actuales

    bool m_newLineDetectEnable;
    bool m_newLineDetected;
    newLineType_t m_newLineType;

    ColaCircular(const ColaCircular&);
    ColaCircular& operator=(const ColaCircular&);

public:

    // Constructor para inicializar la cola con la capacidad dada
    ColaCircular(int tama) : m_idx1(0), m_idx2(0) //, m_cont(0)
	{
		m_size= tama>0? tama:10;
        queue = new T[m_size];
        m_newLineDetectEnable=false;
//       m_newLineDetected=false;
//       m_newLineType=LF;

	}

  //-----------------------------------------------------
    // New line enable
    void enableNewLine(newLineType_t mode=LF)
    {
        m_newLineDetectEnable=true;
        m_newLineDetected=false;
        m_newLineType=mode;
    }

    // New line enable
    void disableNewLine()    { m_newLineDetectEnable=false;}

    bool isNewLine() const  {return m_newLineDetected;}
    void newLineClear() {m_newLineDetected=false;}
    //-----------------------------------------------------



    // Destructor para liberar la memoria
    ~ColaCircular() {
        delete[] queue;
    }

    // Agregar un elemento al final de la cola
    bool push(const T &item);

    //  devuelve y elimina el primer elemento de la cola
    bool pop(T &val);

    // Verificar si la cola está vacía
    bool isEmpty() const {	return (m_idx2-m_idx1) == 0;}

    // Verificar si la cola está llena
    bool isFull() const { return (m_idx2-m_idx1) == m_size;}

    // Obtener el tamaño en la cola
    int size() const { return m_size; }

    // Obtener el número de elementos en la cola
    int qtty() const { return (m_idx2-m_idx1);   }
};


template <typename T>
bool ColaCircular<T>::push(const T &item)
{
	if (isFull())
		return false;	// Buffer lleno

	if ( m_newLineDetectEnable)	// se habilita modo string - deteccion de newLine
								// si bien se podría hacer una especialización para char o uint8_t, lo hacemos generico
	{
		switch (m_newLineType)
		{
		case CR:
			if (item=='\r')
				m_newLineDetected=true;
			break;
		case LF:
			if (item=='\n')
				m_newLineDetected=true;
			break;
		case CR_LF:

			if (item=='\n' && queue[(m_idx2-1)<0?m_size-1:m_idx2-1] == '\r')
				m_newLineDetected=true;
			break;
		}
	}

//	queue[m_idx2] = item;
//	m_idx2 = (m_idx2 + 1) % m_size;
//	m_cont++;

	queue[m_idx2% m_size] = item;
	m_idx2++;
	// ---> Cual es el límite??



return true;
}



template <typename T>
bool ColaCircular<T>::pop(T &val)
{

    if (isEmpty())
		return false;

  //  val = queue[m_idx1];

  // m_idx1 = (m_idx1 + 1) % m_size;
  // m_cont--;

    val = queue[m_idx1%m_size];
    m_idx1++;



 return true;
}



#endif /* COLACIRCULAR_H_ */
#endif /* CC_WORK_MODE */
