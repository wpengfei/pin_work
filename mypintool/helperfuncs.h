

bool isLibRtnName(string name){
  for(int i=0;i<LIB_RTN_NAME_SIZE;++i){
    if(name==LIB_RTN_NAME[i])
      return true;
  }
  return false;
}