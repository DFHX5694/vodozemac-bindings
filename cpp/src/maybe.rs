pub struct Maybe<T>(pub(crate) Vec<Result<T, anyhow::Error>>);

impl<T> Maybe<T> {
    pub fn is_valid(&self) -> bool {
        self.0.len() == 1
    }

    pub fn has_value(&self) -> bool {
        match self.0.last() {
            Some(v) => { v.is_ok() }
            None => { panic!("Maybe is invalid") }
        }
    }

    pub fn take_value(&mut self) -> T {
        match self.0.pop() {
            Some(v) => { v.unwrap() }
            None => { panic!("Maybe is invalid") }
        }
    }

    pub fn take_error(&mut self) -> String {
        match self.0.pop() {
            Some(v) => {
                match v {
                    Err(err) => { format!("{}", err) }
                    Ok(_) => { panic!("Maybe does not have an error") }
                }
            }
            None => { panic!("Maybe is invalid") }
        }
    }
}

impl<T> From<Result<T, anyhow::Error>> for Maybe<T> {
    fn from(v: Result<T, anyhow::Error>) -> Maybe<T> {
        let mut vec = Vec::<Result<T, anyhow::Error>>::new();
        vec.push(v);
        Maybe::<T>(vec)
    }
}
