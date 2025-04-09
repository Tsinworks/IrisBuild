#pragma once

struct FVector
{
  float X;
  float Y;
  float Z;
};

inline FVector operator+(const FVector& A, const FVector& B) {
  FVector R;
  R.X = A.X + B.X;
  R.Y = A.Y + B.Y;
  R.Z = A.Z + B.Z;
  return R;
}