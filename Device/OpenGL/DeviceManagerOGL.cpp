#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#include "Engine/Engine.h"
#include "../DeviceManager.h"

bool CDeviceManager::ms_bIsPointSizeEnabled = false;

std::vector<unsigned int> CDeviceManager::ms_nBufferIDs;
std::vector<unsigned int> CDeviceManager::ms_nCommandListsIDs;
std::vector<unsigned int> CDeviceManager::ms_nConstantsBindingPoint;

unsigned int CDeviceManager::ms_nActiveVAO = 0;


#define BUFFER_OFFSET(i) ((char *)NULL + (i))


void CDeviceManager::CreateDevice()
{
	glewExperimental = 1;
	GLenum ret;
	if ((ret = glewInit()) != GLEW_OK)
	{
		fprintf(stderr, "ERROR: could not start GLEW %s\n", glewGetErrorString(ret));
		return;
	}
}


unsigned int CDeviceManager::CreateCommandList()
{
	unsigned int uID;
	glGenVertexArrays(1, &uID);

	ms_nCommandListsIDs.push_back(uID);

	return uID;
}


void CDeviceManager::DeleteCommandList(unsigned int uID)
{
	glDeleteVertexArrays(1, &uID);
}


void CDeviceManager::BindCommandList(unsigned int uID)
{
	if (ms_nActiveVAO != uID)
	{
		ms_nActiveVAO = uID;

		glBindVertexArray(uID);
	}
}


void CDeviceManager::InitFrame()
{
	ms_nActiveVAO = 0;
	
	size_t size = ms_nConstantsBindingPoint.size();
	ms_nConstantsBindingPoint.clear();
	ms_nConstantsBindingPoint.resize(size);
}


unsigned int CDeviceManager::CreateVertexBuffer(void* pData, size_t size, bool bDynamicDraw)
{
	unsigned int uID;
	glGenBuffers(1, &uID);
	glBindBuffer(GL_ARRAY_BUFFER, uID);
	glBufferData(GL_ARRAY_BUFFER, size, pData, bDynamicDraw ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	ms_nBufferIDs.push_back(uID);

	return uID;
}


void CDeviceManager::FillVertexBuffer(unsigned int nBufferID, void* pData, size_t size, size_t offset)
{
	glBindBuffer(GL_ARRAY_BUFFER, nBufferID);
	glBufferSubData(GL_ARRAY_BUFFER, offset, size, pData);
}


void CDeviceManager::BindVertexBuffer(unsigned int uID, unsigned int Slot, unsigned int nElementCount, CStream::EDataType eType, unsigned int nStride, unsigned int nOffset)
{
	GLenum Type;

	switch (eType)
	{
	case CStream::e_ShortFloat:
		Type = GL_HALF_FLOAT;
		break;

	case CStream::e_Float:
		Type = GL_FLOAT;
		break;

	case CStream::e_Int:
		Type = GL_INT;
		break;

	case CStream::e_ShortInt:
		Type = GL_SHORT;
		break;

	default:
		Type = GL_FLOAT;
		break;
	}

	glEnableVertexAttribArray(Slot);
	glBindBuffer(GL_ARRAY_BUFFER, uID);
	glVertexAttribPointer(Slot, nElementCount, Type, GL_FALSE, nStride, BUFFER_OFFSET(nOffset));
}


void CDeviceManager::UnbindVertexBuffer(unsigned int nSlot)
{
	glEnableVertexAttribArray(nSlot);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void CDeviceManager::DeleteVertexBuffer(unsigned int uID)
{
	glDeleteBuffers(1, &uID);
}


unsigned int CDeviceManager::CreateIndexBuffer(void* pData, size_t size)
{
	unsigned int uID;
	glGenBuffers(1, &uID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, pData, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	ms_nBufferIDs.push_back(uID);

	return uID;
}


void CDeviceManager::BindIndexBuffer(unsigned int uID)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uID);
}


void CDeviceManager::DeleteIndexBuffer(unsigned int uID)
{
	glDeleteBuffers(1, &uID);
}


unsigned int CDeviceManager::CreateConstantBuffer(void* pData, size_t size)
{
	unsigned int uID;
	glGenBuffers(1, &uID);
	glBindBuffer(GL_UNIFORM_BUFFER, uID);
	glBufferData(GL_UNIFORM_BUFFER, size, pData, GL_STATIC_DRAW);

	ms_nBufferIDs.push_back(uID);

	if (uID >= ms_nConstantsBindingPoint.size())
	{
		ms_nConstantsBindingPoint.resize(uID + 1);
		ms_nConstantsBindingPoint[uID] = INVALID_PROGRAM_HANDLE;
	}

	return uID;
}


void CDeviceManager::FillConstantBuffer(unsigned int uID, void* pData, size_t size)
{
	void* pMapped = glMapNamedBufferRange(uID, 0, size, GL_MAP_WRITE_BIT);
	memcpy(pMapped, pData, size);
	glUnmapNamedBuffer(uID);
}


void CDeviceManager::BindConstantBuffer(unsigned int uID, ProgramHandle pid, unsigned int Slot)
{
	if (ms_nConstantsBindingPoint[uID] != pid || CShader::ms_nConstantsBindingPoint[pid] != uID)
	{
		ms_nConstantsBindingPoint[uID] = pid;
		CShader::ms_nConstantsBindingPoint[pid] = uID;

		char str[32] = "";
		sprintf_s(str, "cb%d", Slot);

		GLuint blockIndex = glGetUniformBlockIndex(pid, str);
		glUniformBlockBinding(pid, blockIndex, Slot);
		glBindBufferBase(GL_UNIFORM_BUFFER, Slot, uID);
	}
}


void CDeviceManager::DeleteConstantBuffer(unsigned int uID)
{
	glDeleteBuffers(1, &uID);
}


void CDeviceManager::EnableConservativeRasterization()
{
	glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
}


void CDeviceManager::DisableConservativeRasterization()
{
	glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
}


void* CDeviceManager::MapBuffer(unsigned int nBufferID, unsigned int eAccess)
{
	unsigned int nFlags = GL_READ_WRITE;

	if (eAccess == e_Write)
		nFlags = GL_WRITE_ONLY;

	if (eAccess == e_Read)
		nFlags = GL_READ_ONLY;

	return glMapNamedBuffer(nBufferID, nFlags);
}


void CDeviceManager::UnmapBuffer(unsigned int nBufferID)
{
	glUnmapNamedBuffer(nBufferID);
}


unsigned int CDeviceManager::GetResourceSlot(ProgramHandle nPID, const char* pResourceName)
{
	return glGetProgramResourceIndex(nPID, GL_SHADER_STORAGE_BLOCK, pResourceName);
}


unsigned int CDeviceManager::CreateStorageBuffer(void* pData, size_t size)
{
	unsigned int uID;

	glGenBuffers(1, &uID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, uID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, pData, GL_DYNAMIC_COPY);

	ms_nBufferIDs.push_back(uID);

	return uID;
}


void CDeviceManager::FillStorageBuffer(unsigned int uID, void* pData, size_t size)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, uID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, pData, GL_DYNAMIC_COPY);
}


void CDeviceManager::DeleteStorageBuffer(unsigned int uID)
{
	glDeleteBuffers(1, &uID);
}


void CDeviceManager::BindStorageBuffer(unsigned int nBufferID, unsigned int nSlot, size_t nElementSize, unsigned int nOffset, unsigned int nCount)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nBufferID);
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, nSlot, nBufferID, nOffset * nElementSize, nCount * nElementSize);
}


void CDeviceManager::DrawInstanced(EPrimitiveTopology eTopology, int nPrimitiveOffset, int nPrimitiveCount, int nInstanceCount)
{
	if (eTopology == e_TriangleList)
		glDrawArraysInstanced(GL_TRIANGLES, nPrimitiveOffset, 3 * nPrimitiveCount, nInstanceCount);

	else if (eTopology == e_TriangleStrip)
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, nPrimitiveOffset, 2 + nPrimitiveCount, nInstanceCount);

	else if (eTopology == e_TriangleFan)
		glDrawArraysInstanced(GL_TRIANGLE_FAN, nPrimitiveOffset, 2 + nPrimitiveCount, nInstanceCount);

	else if (eTopology == e_LineList)
		glDrawArraysInstanced(GL_LINES, nPrimitiveOffset, 2 * nPrimitiveCount, nInstanceCount);

	else if (eTopology == e_PointList)
	{
		if (!ms_bIsPointSizeEnabled)
		{
			glEnable(GL_POINT_SPRITE);
			glEnable(GL_PROGRAM_POINT_SIZE);
			ms_bIsPointSizeEnabled = true;
		}

		glDrawArraysInstanced(GL_POINTS, nPrimitiveOffset, nPrimitiveCount, nInstanceCount);
	}

	else if (eTopology == e_TrianglePatch)
	{
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		glDrawArraysInstanced(GL_PATCHES, nPrimitiveOffset, 3 * nPrimitiveCount, nInstanceCount);
	}
}


void CDeviceManager::DrawInstancedIndexed(EPrimitiveTopology eTopology, int nPrimitiveCount, int nPrimitiveOffset, int nInstanceCount)
{
	if (eTopology == e_TriangleList)
		glDrawElementsInstanced(GL_TRIANGLES, 3 * nPrimitiveCount, GL_UNSIGNED_INT, BUFFER_OFFSET(nPrimitiveOffset * sizeof(unsigned int)), nInstanceCount);

	else if (eTopology == e_TrianglePatch)
	{
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		glDrawElementsInstanced(GL_PATCHES, 3 * nPrimitiveCount, GL_UNSIGNED_INT, BUFFER_OFFSET(nPrimitiveOffset * sizeof(unsigned int)), nInstanceCount);
	}
}


void CDeviceManager::DrawIndexed(EPrimitiveTopology eTopology, int nTriangleCount, unsigned int nOffset)
{
	if (eTopology == e_TriangleList)
		glDrawElements(GL_TRIANGLES, 3 * nTriangleCount, GL_UNSIGNED_INT, BUFFER_OFFSET(nOffset * sizeof(unsigned int)));

	else if (eTopology == e_TrianglePatch)
	{
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		glDrawElements(GL_PATCHES, 3 * nTriangleCount, GL_UNSIGNED_INT, BUFFER_OFFSET(nOffset * sizeof(unsigned int)));
	}
}


void CDeviceManager::Draw(EPrimitiveTopology eTopology, int nOffset, int nPrimitiveCount)
{
	if (eTopology == e_TriangleList)
		glDrawArrays(GL_TRIANGLES, nOffset, 3 * nPrimitiveCount);

	else if (eTopology == e_TriangleStrip)
		glDrawArrays(GL_TRIANGLE_STRIP, nOffset, 2 + nPrimitiveCount);

	else if (eTopology == e_TriangleFan)
		glDrawArrays(GL_TRIANGLE_FAN, nOffset, 2 + nPrimitiveCount);

	else if (eTopology == e_LineList)
		glDrawArrays(GL_LINES, nOffset, 2 * nPrimitiveCount);

	else if (eTopology == e_PointList)
	{
		if (!ms_bIsPointSizeEnabled)
		{
			glEnable(GL_POINT_SPRITE);
			glEnable(GL_PROGRAM_POINT_SIZE);
			ms_bIsPointSizeEnabled = true;
		}
		
		glDrawArrays(GL_POINTS, nOffset, nPrimitiveCount);
	}

	else if (eTopology == e_TrianglePatch)
	{
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		glDrawArrays(GL_PATCHES, nOffset, 3 * nPrimitiveCount);
	}
}


void CDeviceManager::Dispatch(unsigned int nThreadsX, unsigned int nThreadsY, unsigned int nThreadsZ)
{
	glDispatchCompute(nThreadsX, nThreadsY, nThreadsZ);
}


void CDeviceManager::SetMemoryBarrierOnBufferAccess()
{
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}


void CDeviceManager::SetMemoryBarrierOnImageAccess()
{
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}


void CDeviceManager::SetMemoryBarrierOnBufferMapping()
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}


void CDeviceManager::SetMemoryBarrier()
{
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}


void CDeviceManager::SetUniform(ProgramHandle nPID, const char* pName, float fValue)
{
	GLuint uniform = glGetUniformLocation(nPID, pName);
	glUniform1f(uniform, fValue);
}


void CDeviceManager::SetUniform(ProgramHandle nPID, const char* pName, int nValue)
{
	GLuint uniform = glGetUniformLocation(nPID, pName);
	glUniform1i(uniform, nValue);
}


void CDeviceManager::SetUniform(ProgramHandle nPID, const char* pName, unsigned int nValue)
{
	GLuint uniform = glGetUniformLocation(nPID, pName);
	glUniform1ui(uniform, nValue);
}


void CDeviceManager::ClearBuffers()
{
	std::vector<unsigned int>::iterator it;

	for (it = ms_nBufferIDs.begin(); it < ms_nBufferIDs.end(); it++)
	{
		glDeleteBuffers(1, &(*it));
	}

	ms_nBufferIDs.clear();
}


void CDeviceManager::ClearCommandLists()
{
	std::vector<unsigned int>::iterator it;

	for (it = ms_nBufferIDs.begin(); it < ms_nBufferIDs.end(); it++)
	{
		glDeleteVertexArrays(1, &(*it));
	}

	ms_nCommandListsIDs.clear();
}
