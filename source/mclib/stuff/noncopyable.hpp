//===========================================================================//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//
/*
This header and class is not used
*/

#pragma once

#ifndef NONCOPYABLE_HPP
#define NONCOPYABLE_HPP

namespace Stuff
{
class Noncopyable
{
  protected:
	Noncopyable() {}

  private:
	Noncopyable(const Noncopyable&);
	const Noncopyable& operator=(const Noncopyable&);
};
};
#endif // NONCOPYABLE_HPP
